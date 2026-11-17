# padshell devlog

The development log, built with [Hugo](https://gohugo.io/) into two outputs
from the same Markdown:

* `public/`: the website, published to GitHub Pages by
  `.github/workflows/devlog.yml` on every push to `main` that touches `site/`.
* `public/gemini/`: the Gemini capsule. CI attaches it to each run as the
  `gemini-capsule` artifact. It is served at `gemini://gemini.hjalves.dev/padshell/`
  (`params.capsulePath`); copy it there with
  `rsync -a --delete public/gemini/ dk@atom:/srv/gemini/gemini.hjalves.dev/padshell/`.

## Writing an entry

Each entry is a folder (a page bundle) with its images next to it:

```
content/devlog/my-entry/index.md
content/devlog/my-entry/screenshot.png
```

```markdown
---
title: "My entry"
date: 2026-11-20T18:00:00Z
summary: "One sentence for the entry list and link previews."
draft: true          # remove to publish
---
```

Entries with `draft: true` are left out of the published site.

## Markdown that also works as gemtext

Gemtext is line-based, so `layouts/_partials/gemtext.html` converts a subset
of Markdown. Stick to:

* `#`, `##`, `###` headings; paragraphs (wrap lines freely); `-` lists;
  `>` quotes; ``` code blocks.
* Images on a line of their own: `![caption](file.png)`.
* Links that matter on a line of their own: `[text](url)`. Inline links
  work too; in gemtext they become `=>` lines after the paragraph.
* `**bold**`, `*em*` and `` `code` `` are fine; the markers are dropped in
  gemtext.

Avoid tables, raw HTML, shortcodes and nested lists.

## Previewing

```sh
hugo server -D                       # website at http://localhost:1313/padshell/
hugo -D && tools/serve-gemini.py     # capsule at gemini://<this machine>/padshell/
```

`serve-gemini.py` is a small preview server with a self-signed certificate;
open it from padshell on the Switch with X and the machine's LAN address
followed by `/padshell/`.
