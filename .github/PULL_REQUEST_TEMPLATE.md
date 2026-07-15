<!-- Thanks for contributing to open-snaphak! Keep each PR focused on a single change. -->

## What this changes

<!-- A short description of the change and why it's needed. -->

## Checklist

- [ ] Clean-room: this is my own reverse-engineering / implementation -- no decompiled or copyrighted DOOM or
      original-SnapHak content is pasted into the repo.
- [ ] No binaries added (`.dll` / `.exe` / `.obj` / `.pdb` / `.zip` ...) -- the source is the only deliverable.
- [ ] Source stays pure ASCII (`.c` / `.h` / `.cpp` / `.ps1`).
- [ ] Built + tested locally: `build.ps1` -> `package.ps1` -> `snaphak install --local dist`, and I ran the Go
      (`gofmt` / `go vet` / `go test`) and C (`tests\run-tests.ps1`) suites, and confirmed it works in DOOM.
- [ ] Docs updated in this PR for any behavior change (see `docs/contributing.md` section 9).

<!-- On every PR, CI runs a secretless security gate + a Windows build/test, and a maintainer reviews before merge. -->
