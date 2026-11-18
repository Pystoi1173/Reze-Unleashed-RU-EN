# Upstream GitHub Actions archive

The eight workflows in this directory are preserved from the Unleashed base commit with their contents unchanged and a `.disabled` suffix. They are outside `.github/workflows`, so GitHub does not run them.

This standalone RU/EN prerelease currently uses a locally built and device-tested update package. The upstream automation assumes upstream branch names, contribution policies, build configurations, permissions, and release procedures. It has not been adapted or validated for this project. In particular, upstream scheduled issue locking and release-triggered commits should not run automatically in this repository.

Before enabling a workflow, review its triggers, repository permissions, external actions, build options, artifact names and flash/C2 budget checks. Adapt it for RU/EN, then deliberately restore the desired YAML file to `.github/workflows`. The upstream `.drone.yml` is also unconfigured for this repository and should be reviewed before connecting Drone.
