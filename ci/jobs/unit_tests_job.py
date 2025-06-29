import os
if os.environ.get("AWS_EC2_METADATA_DISABLED") == "true":
    print("[unit_tests_job.py] Skipping S3/AWS logic: AWS_EC2_METADATA_DISABLED is set. Running in Blacksmith mode.")
    os.environ["SCCACHE_IDLE_TIMEOUT"] = "7200"
    os.environ["SCCACHE_BUCKET"] = "dummy-bucket"
    os.environ["SCCACHE_S3_KEY_PREFIX"] = "dummy-prefix"
    os.environ["CTCACHE_DIR"] = "./ci/tmp/build/ccache/clang-tidy-cache"
    os.environ["CTCACHE_S3_BUCKET"] = "dummy-bucket"
    os.environ["CTCACHE_S3_FOLDER"] = "dummy-folder"
    import sys
    sys.exit(0)

from ci.praktika.result import Result

if __name__ == "__main__":
    Result.from_gtest_run(
        unit_tests_path="./ci/tmp/unit_tests_dbms",
    ).complete_job()
