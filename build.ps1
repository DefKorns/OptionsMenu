$ErrorActionPreference = "Stop"
if (-not (Test-Path "vendor/hmod-build/toolchain-image.sh")) {
    Write-Host "==> Fetching vendor/hmod-build submodule..."
    git submodule update --init vendor/hmod-build
}

# same pin as vendor/hmod-build/toolchain-image.sh
$IMAGE = $env:TOOLCHAIN_IMAGE
if (-not $IMAGE) {
    $IMAGE = (Select-String -Path "vendor/hmod-build/toolchain-image.sh" -Pattern 'TOOLCHAIN_IMAGE:-([^}]+)').Matches[0].Groups[1].Value
}
docker image inspect $IMAGE *> $null
if ($LASTEXITCODE -ne 0) {
    Write-Host "==> Pulling $IMAGE..."
    docker pull $IMAGE
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Docker pull failed (exit code $LASTEXITCODE)." -ForegroundColor Red
        exit $LASTEXITCODE
    }
}

# the outermost work tree is mounted, so a submodule checkout's gitdir resolves for git describe
$here = (git rev-parse --show-toplevel) + "/" + (git rev-parse --show-prefix)
$root = git rev-parse --show-toplevel
while ($super = git -C $root rev-parse --show-superproject-working-tree) { $root = $super }
$rel = $here.Substring($root.Length).Trim("/")

Write-Host "==> Running make inside the container..."
docker run --rm -v "${root}:/ws" -w "/ws/$rel" $IMAGE `
    sh -c 'git config --global --add safe.directory ''*'' 2>/dev/null; exec make CROSS_PREFIX=arm-linux-gnueabihf- $@' make @args
if ($LASTEXITCODE -ne 0) {
    Write-Host "make failed (exit code $LASTEXITCODE)." -ForegroundColor Red
    exit $LASTEXITCODE
}

Write-Host "==> Build complete. Files in out\:"
Get-ChildItem out
