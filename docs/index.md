# mag-usb Documentation

Welcome to the mag-usb documentation site. This project provides a USB-connected interface and tools for working with magnetometer sensors.

Use the navigation to the left or the links below to explore the documentation.

## Quick links

- Getting started: see [Getting Started](Getting-Started.md)
- Hardware setup: see [Hardware Setup](Hardware-Setup.md)
- Configuring the software: see [Configuration](Configuration.md)
- Data format reference: see [Data Format](Data-Format.md)
- Orientation and axes: see [Orientation and Axes](Orientation-and-Axes.md)
- Troubleshooting: see [Troubleshooting](Troubleshooting.md)
- Development notes: see [Development](Development.md)

## Local preview

You can build and preview this site locally with MkDocs. First, create a virtual environment and install the docs dependencies:

```bash
python3 -m venv .venv
. .venv/bin/activate
pip install -r docs/requirements.txt
```

Then serve the site locally:

```bash
mkdocs serve
```

Open the URL printed by MkDocs (typically http://127.0.0.1:8000/) to preview.

## Read the Docs

This repository is configured for Read the Docs using `.readthedocs.yaml` and `mkdocs.yml`.

To publish the site on Read the Docs:

1. Push these files to your default branch.
2. Log in to https://readthedocs.org/ and import the project by pointing it at your repository.
3. In the RTD project settings, ensure the configuration file path is `.readthedocs.yaml`.
4. Trigger a build. RTD will install `docs/requirements.txt` and build the MkDocs site.
5. Once built, add the RTD URL to your repository README.

If your repository is hosted publicly (e.g., GitHub/GitLab), update `repo_url` in `mkdocs.yml` to enable "Edit this page" links.
