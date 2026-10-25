# Security Policy

## Supported version

Security fixes are intended for the current code on the `main` branch and the latest production deployment.

## Reporting a vulnerability

Please do **not** publish exploitable vulnerability details in a public issue.

If GitHub private vulnerability reporting is enabled for this repository, use the repository's **Security → Advisories → Report a vulnerability** workflow. If that option is unavailable, contact the repository maintainer privately through the maintainer's GitHub profile and ask for a secure reporting channel.

When reporting, include:

- A concise description of the issue and its potential impact.
- Affected commit, deployment, endpoint, or configuration, if known.
- Minimal reproduction steps or a proof of concept that does not expose real user data.
- Any suggested mitigation.

Please allow reasonable time for investigation and a fix before publicly disclosing details. Do not include secrets, access tokens, or personal data in the report.

## Deployment security notes

- The API currently has no authentication and is intended as a stateless solver service.
- Do not commit production secrets or local `.env` files.
- Keep dependencies and the deployment base image updated.
- Review rate limiting and abuse protection before exposing the service to high-volume traffic.
