S3_BUCKET_NAME = "clickhouse-builds"
S3_REPORT_BUCKET_NAME = "clickhouse-test-reports"
S3_BUCKET_HTTP_ENDPOINT = "clickhouse-builds.s3.amazonaws.com"
S3_REPORT_BUCKET_HTTP_ENDPOINT = "s3.amazonaws.com/clickhouse-test-reports"

# Disable AWS EC2 metadata service calls on blacksmith runners
AWS_EC2_METADATA_DISABLED = "true"

class RunnerLabels:
    # Style check runners (light jobs)
    STYLE_CHECK_AMD = ["blacksmith-2vcpu-ubuntu-2204"]
    STYLE_CHECK_ARM = ["blacksmith-2vcpu-ubuntu-2204-arm"]
    # Builder runners (heavy build jobs) - using 32vcpu for more resources
    BUILDER_AMD = ["blacksmith-32vcpu-ubuntu-2204"]
    BUILDER_ARM = ["blacksmith-32vcpu-ubuntu-2204-arm"]


MAIN_BRANCH = "master"
S3_ARTIFACT_PATH = S3_BUCKET_NAME
CI_CONFIG_RUNS_ON = RunnerLabels.STYLE_CHECK_ARM

ENABLE_MULTIPLATFORM_DOCKER_IN_ONE_JOB = False
DOCKER_MERGE_RUNS_ON = RunnerLabels.STYLE_CHECK_AMD
DOCKER_BUILD_ARM_RUNS_ON = RunnerLabels.STYLE_CHECK_ARM
DOCKER_BUILD_AMD_RUNS_ON = RunnerLabels.STYLE_CHECK_AMD

CACHE_S3_PATH = f"{S3_BUCKET_NAME}/ci_ch_cache"
HTML_S3_PATH = S3_REPORT_BUCKET_NAME
S3_BUCKET_TO_HTTP_ENDPOINT = {
    S3_BUCKET_NAME: S3_BUCKET_HTTP_ENDPOINT,
    S3_REPORT_BUCKET_NAME: S3_REPORT_BUCKET_HTTP_ENDPOINT,
}
ENABLE_ARTIFACTS_REPORT = False

COMPRESS_THRESHOLD_MB = 32
TEXT_CONTENT_EXTENSIONS = [".txt", ".log", ".err", ".out", ".tsv", ".csv", ".json"]

DOCKERHUB_USERNAME = "robotclickhouse"
DOCKERHUB_SECRET = "dockerhub_robot_password"

CI_DB_DB_NAME = "default"
CI_DB_TABLE_NAME = "checks"
SECRET_CI_DB_URL = "clickhouse-test-stat-url"
SECRET_CI_DB_USER = "clickhouse-test-stat-login"
SECRET_CI_DB_PASSWORD = "clickhouse-test-stat-password"

USE_CUSTOM_GH_AUTH = False
SECRET_GH_APP_ID: str = "woolenwolf_gh_app.clickhouse-app-id"
SECRET_GH_APP_PEM_KEY: str = "woolenwolf_gh_app.clickhouse-app-key"

INSTALL_PYTHON_REQS_FOR_NATIVE_JOBS = ""

DISABLED_WORKFLOWS = [
    "new_pull_request.py",
]

DEFAULT_LOCAL_TEST_WORKFLOW = "pull_request.py"
READY_FOR_MERGE_CUSTOM_STATUS_NAME = "Mergeable Check"
