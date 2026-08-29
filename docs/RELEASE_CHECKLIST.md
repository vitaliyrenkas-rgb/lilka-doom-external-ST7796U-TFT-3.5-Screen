# Release checklist — Doom for Lilka / ST7796U v1.0.0

Full command-by-command procedure: **[FINAL_RELEASE_COMMANDS.md](../FINAL_RELEASE_COMMANDS.md)**

Before Publish:

- [ ] game code unchanged after the already-passed regression;
- [ ] release docs committed;
- [ ] no `.wad` tracked or staged;
- [ ] no old `.patch` / `.zip` staged;
- [ ] `main` fast-forwarded to the final release commit;
- [ ] repository visibility changed to **PUBLIC**;
- [ ] `@And3rson` and `@ozkl` are present as raw mentions in `GITHUB_RELEASE_BODY.md`;
- [ ] annotated tag `doom-st7796u-v1.0.0` created and pushed;
- [ ] exact tested `doom.bin` selected, or rebuilt from the tagged source if the tested binary is unavailable;
- [ ] `doom.bin.sha256` generated;
- [ ] GitHub Release assets contain `doom.bin` + checksum and NO WAD;
- [ ] README/gallery links render correctly after publish.

After Publish:

- [ ] open the public release page;
- [ ] verify upstream credits are visible;
- [ ] verify release assets download;
- [ ] stop changing v1.0.0 unless a release-blocking bug appears.
