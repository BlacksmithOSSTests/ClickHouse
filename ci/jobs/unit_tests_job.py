import os
if os.environ.get("AWS_EC2_METADATA_DISABLED") == "true":
    print("[unit_tests_job.py] Skipping AWS/SSM/Azure logic: AWS_EC2_METADATA_DISABLED is set. Running in Blacksmith mode.")

from ci.praktika.result import Result

if __name__ == "__main__":
    Result.from_gtest_run(
        unit_tests_path="./ci/tmp/unit_tests_dbms",
    ).complete_job()
