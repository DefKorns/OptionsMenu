/*
 *  Copyright (C) 2026 DefKorns
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/* HTTPS downloader: BusyBox wget can't do GitHub's TLS.
 * Batch line: <out_path>\t<url>[\t<fallback_url>...]; result on stdout: "<rc> <out_path>".
 * Progress on stderr: "\r<label> <pct>% (<done>/<total> MB)". */

#include <ctype.h>
#include <curl/curl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <time.h>
#include <unistd.h>

extern const unsigned char _binary_cacert_pem_start[];
extern const unsigned char _binary_cacert_pem_end[];

static const char *const CA_PATH = "/tmp/.om_downloader_ca.pem";

enum { CONNECT_TIMEOUT_S = 10, STALL_TIMEOUT_S = 30, MAX_REDIRECTS = 10 };
enum { PROGRESS_INTERVAL_MS = 250, LABEL_MAX_BYTES = 24 };
enum { MAX_URLS = 8, MAX_KEEP = 8, PATH_BYTES = 512, URL_BYTES = 1024, LINE_BYTES = 4096 };
enum { REMOVAL_POLL_US = 200000 };
enum exit_code { EXIT_OK = 0, EXIT_FAILED = 1, EXIT_USAGE = 2, EXIT_NO_SPACE = 3, EXIT_CANCELLED = 143 };
enum fetch_result { FETCH_FAILED, FETCH_OK, FETCH_WRITE_ERROR, FETCH_CANCELLED };

static volatile sig_atomic_t cancelled = 0;

struct progress {
    int enabled;
    char label[LABEL_MAX_BYTES + 1];
    long last_ms;
    const char *free_label;
    char dir[PATH_BYTES];
    const char *storage;
};

static void on_cancel(int sig) {
    (void)sig;
    cancelled = 1;
}

static void install_signal_handlers(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_cancel;
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGHUP, &sa, NULL);
    /* survive a closed reader so the .part still gets removed */
    signal(SIGPIPE, SIG_IGN);
}

static long monotonic_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

static void ensure_ca_bundle(void) {
    FILE *f = fopen(CA_PATH, "rb");
    if (f) {
        fclose(f);
        return;
    }
    f = fopen(CA_PATH, "wb");
    if (!f) return;
    fwrite(_binary_cacert_pem_start, 1, (size_t)(_binary_cacert_pem_end - _binary_cacert_pem_start), f);
    fclose(f);
}

/* without an all-caps "SNES." prefix or the archive extension; cut on a UTF-8 boundary */
static void make_label(char *dst, const char *out_path) {
    static const char *const archive_exts[] = { ".tar.gz", ".tgz", ".gz", ".zip" };
    const char *base = strrchr(out_path, '/');
    base = base ? base + 1 : out_path;
    const char *dot = strchr(base, '.');
    if (dot && dot > base && dot[1]) {
        const char *c = base;
        while (c < dot && isupper((unsigned char)*c)) c++;
        if (c == dot) base = dot + 1;
    }
    size_t len = strlen(base);
    for (size_t i = 0; i < sizeof archive_exts / sizeof *archive_exts; i++) {
        const size_t ext_len = strlen(archive_exts[i]);
        if (len > ext_len && strcmp(base + len - ext_len, archive_exts[i]) == 0) {
            len -= ext_len;
            break;
        }
    }
    if (len > LABEL_MAX_BYTES) {
        len = LABEL_MAX_BYTES;
        while (len > 0 && ((unsigned char)base[len] & 0xC0) == 0x80) len--;
    }
    memcpy(dst, base, len);
    dst[len] = '\0';
}

static void make_dir(char *dst, size_t dst_size, const char *out_path) {
    const char *slash = strrchr(out_path, '/');
    if (!slash) {
        snprintf(dst, dst_size, ".");
        return;
    }
    const int len = slash == out_path ? 1 : (int)(slash - out_path);
    snprintf(dst, dst_size, "%.*s", len, out_path);
}

static const char *storage_name(const char *dir) {
    struct stat media, root, target;
    if (stat("/media", &media) != 0 || stat("/", &root) != 0 || stat(dir, &target) != 0) return "NAND";
    return media.st_dev != root.st_dev && target.st_dev == media.st_dev ? "USB" : "NAND";
}

static void format_free_space(char *dst, size_t dst_size, const struct progress *p) {
    struct statvfs vfs;
    dst[0] = '\0';
    if (!p->free_label || statvfs(p->dir, &vfs) != 0) return;
    const double mb = (double)vfs.f_bavail * vfs.f_frsize / (1024.0 * 1024.0);
    if (mb >= 1024.0)
        snprintf(dst, dst_size, " | %s %.1f GB %s", p->storage, mb / 1024.0, p->free_label);
    else
        snprintf(dst, dst_size, " | %s %.1f MB %s", p->storage, mb, p->free_label);
}

static void print_progress(const struct progress *p, curl_off_t now, curl_off_t total) {
    const double mb = 1024.0 * 1024.0;
    char space[64];
    format_free_space(space, sizeof(space), p);
    if (total > 0)
        fprintf(stderr, "\r%s %3d%% (%.1f/%.1f MB)%s\n", p->label, (int)(now * 100 / total), now / mb, total / mb, space);
    else
        fprintf(stderr, "\r%s %.1f MB%s\n", p->label, now / mb, space);
}

static int on_transfer(void *userdata, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
    (void)ultotal;
    (void)ulnow;
    if (cancelled) return 1;
    struct progress *p = userdata;
    if (!p->enabled || dlnow == 0) return 0;
    long now = monotonic_ms();
    if (now - p->last_ms < PROGRESS_INTERVAL_MS) return 0;
    p->last_ms = now;
    print_progress(p, dlnow, dltotal);
    return 0;
}

static CURL *make_handle(struct progress *p) {
    CURL *c = curl_easy_init();
    if (!c) return NULL;
    curl_easy_setopt(c, CURLOPT_CAINFO, CA_PATH);
    curl_easy_setopt(c, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(c, CURLOPT_MAXREDIRS, (long)MAX_REDIRECTS);
    curl_easy_setopt(c, CURLOPT_FAILONERROR, 1L);
    curl_easy_setopt(c, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(c, CURLOPT_USERAGENT, "om_downloader/1.0");
    curl_easy_setopt(c, CURLOPT_CONNECTTIMEOUT, (long)CONNECT_TIMEOUT_S);
    curl_easy_setopt(c, CURLOPT_LOW_SPEED_LIMIT, 1L);
    curl_easy_setopt(c, CURLOPT_LOW_SPEED_TIME, (long)STALL_TIMEOUT_S);
    /* also polls the cancel flag, so it stays on without --progress */
    curl_easy_setopt(c, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(c, CURLOPT_XFERINFOFUNCTION, on_transfer);
    curl_easy_setopt(c, CURLOPT_XFERINFODATA, p);
    return c;
}

/* %, #, ? and & pass through: callers escape them in names */
static int normalize_url(char *dst, size_t dst_size, const char *src) {
    static const char hex[] = "0123456789ABCDEF";
    size_t o = 0;
    for (const unsigned char *s = (const unsigned char *)src; *s; s++) {
        int encode = *s <= 0x20 || *s >= 0x7F || strchr("\"<>\\^`{|}", *s);
        size_t need = encode ? 3 : 1;
        if (o + need >= dst_size) return 0;
        if (encode) {
            dst[o++] = '%';
            dst[o++] = hex[*s >> 4];
            dst[o++] = hex[*s & 0x0F];
        } else {
            dst[o++] = (char)*s;
        }
    }
    dst[o] = '\0';
    return 1;
}

static size_t write_to_file(void *ptr, size_t size, size_t nmemb, void *userdata) {
    return fwrite(ptr, size, nmemb, (FILE *)userdata);
}

static enum fetch_result download_to(CURL *c, struct progress *p, const char *url, const char *out_path) {
    char part_path[PATH_BYTES];
    char safe_url[URL_BYTES];
    if (!normalize_url(safe_url, sizeof(safe_url), url)) return FETCH_FAILED;
    if (snprintf(part_path, sizeof(part_path), "%s.part", out_path) >= (int)sizeof(part_path)) return FETCH_WRITE_ERROR;
    FILE *f = fopen(part_path, "wb");
    if (!f) return FETCH_WRITE_ERROR;
    p->last_ms = 0;
    curl_easy_setopt(c, CURLOPT_URL, safe_url);
    curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, write_to_file);
    curl_easy_setopt(c, CURLOPT_WRITEDATA, f);
    CURLcode res = curl_easy_perform(c);
    int closed = fclose(f) == 0;
    if (res == CURLE_OK && closed && rename(part_path, out_path) == 0) {
        if (p->enabled) {
            curl_off_t size = 0;
            curl_easy_getinfo(c, CURLINFO_SIZE_DOWNLOAD_T, &size);
            print_progress(p, size, size);
        }
        return FETCH_OK;
    }
    remove(part_path);
    if (cancelled) return FETCH_CANCELLED;
    return (res == CURLE_WRITE_ERROR || !closed) ? FETCH_WRITE_ERROR : FETCH_FAILED;
}

static enum fetch_result fetch(CURL *c, struct progress *p, const char *out_path, char *const *urls, int url_count) {
    make_label(p->label, out_path);
    make_dir(p->dir, sizeof(p->dir), out_path);
    p->storage = storage_name(p->dir);
    enum fetch_result r = FETCH_FAILED;
    for (int i = 0; i < url_count && r == FETCH_FAILED; i++) r = download_to(c, p, urls[i], out_path);
    return r;
}

static enum exit_code result_code(enum fetch_result r) {
    switch (r) {
        case FETCH_OK: return EXIT_OK;
        case FETCH_WRITE_ERROR: return EXIT_NO_SPACE;
        case FETCH_CANCELLED: return EXIT_CANCELLED;
        default: return EXIT_FAILED;
    }
}

static int split_tabs(char *line, char **fields, int max_fields) {
    int n = 0;
    char *save = NULL;
    for (char *tok = strtok_r(line, "\t", &save); tok && n < max_fields; tok = strtok_r(NULL, "\t", &save)) fields[n++] = tok;
    return n;
}

/* gives up if the consumer died */
static int wait_until_removed(const char *path, pid_t consumer) {
    while (path[0] && access(path, F_OK) == 0) {
        if (cancelled || getppid() != consumer) return 0;
        usleep(REMOVAL_POLL_US);
    }
    return 1;
}

static enum exit_code fetch_batch(CURL *c, struct progress *p, const char *list_path, int keep) {
    FILE *list = fopen(list_path, "r");
    if (!list) return EXIT_FAILED;
    char line[LINE_BYTES];
    char held[MAX_KEEP][PATH_BYTES];
    memset(held, 0, sizeof(held));
    int slot = 0;
    enum exit_code code = EXIT_OK;
    const pid_t consumer = getppid();
    while (fgets(line, sizeof(line), list)) {
        line[strcspn(line, "\r\n")] = '\0';
        char *fields[1 + MAX_URLS];
        int n = split_tabs(line, fields, 1 + MAX_URLS);
        if (n < 2) continue;
        if (keep > 0) {
            if (!wait_until_removed(held[slot], consumer)) {
                code = EXIT_CANCELLED;
                break;
            }
            snprintf(held[slot], sizeof(held[slot]), "%s", fields[0]);
        }
        code = result_code(fetch(c, p, fields[0], fields + 1, n - 1));
        if (keep > 0) {
            if (code != EXIT_OK) held[slot][0] = '\0';
            slot = (slot + 1) % keep;
        }
        if (code == EXIT_CANCELLED) break;
        if (printf("%d %s\n", code, fields[0]) < 0 || fflush(stdout) != 0) {
            code = EXIT_CANCELLED;
            break;
        }
        if (code == EXIT_NO_SPACE) break;
    }
    fclose(list);
    return (code == EXIT_NO_SPACE || code == EXIT_CANCELLED) ? code : EXIT_OK;
}

static void usage(const char *prog) {
    fprintf(stderr,
            "usage:\n"
            "  %s [--progress [--free-label <text>]] <out_path> <url> [fallback_url...]\n"
            "  %s [--progress [--free-label <text>]] [--keep N] --batch <list_file>\n",
            prog, prog);
}

int main(int argc, char **argv) {
    struct progress p;
    memset(&p, 0, sizeof(p));
    const char *batch = NULL;
    int keep = 0;
    int i = 1;
    for (; i < argc && strncmp(argv[i], "--", 2) == 0; i++) {
        if (strcmp(argv[i], "--progress") == 0) {
            p.enabled = 1;
        } else if (strcmp(argv[i], "--free-label") == 0 && i + 1 < argc) {
            p.free_label = argv[++i];
        } else if (strcmp(argv[i], "--batch") == 0 && i + 1 < argc) {
            batch = argv[++i];
        } else if (strcmp(argv[i], "--keep") == 0 && i + 1 < argc) {
            keep = atoi(argv[++i]);
            if (keep < 0 || keep > MAX_KEEP) {
                usage(argv[0]);
                return EXIT_USAGE;
            }
        } else {
            usage(argv[0]);
            return EXIT_USAGE;
        }
    }
    const int url_count = argc - i - 1;
    if (batch ? i != argc : (url_count < 1 || url_count > MAX_URLS)) {
        usage(argv[0]);
        return EXIT_USAGE;
    }

    install_signal_handlers();
    curl_global_init(CURL_GLOBAL_DEFAULT);
    ensure_ca_bundle();
    CURL *c = make_handle(&p);
    if (!c) {
        curl_global_cleanup();
        return EXIT_FAILED;
    }

    enum exit_code code = batch ? fetch_batch(c, &p, batch, keep) : result_code(fetch(c, &p, argv[i], argv + i + 1, url_count));

    curl_easy_cleanup(c);
    curl_global_cleanup();
    return code;
}
