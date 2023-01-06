import pytest
import os
import sys
import re
from datetime import datetime
from unittest.mock import patch, MagicMock, mock_open
import botocore

sys.path.append(os.path.join(os.path.dirname(os.path.realpath(__file__)), '.'))
import aws_utils as utils

sys.path.append(os.path.join(os.path.dirname(os.path.realpath(__file__)), '..', 'buckets_s3'))
import aws_bucket
import server_access


TEST_LOG_SERVER_ACCESS_FULL_PATH = '2021-04-29-09-12-25-F123456789012345'
utils.LIST_OBJECT_V2_NO_PREFIXES['Contents'][0]['Key'] = TEST_LOG_SERVER_ACCESS_FULL_PATH

@patch('aws_bucket.AWSCustomBucket.__init__')
def test_AWSServerAccess___init__(mock_custom_bucket):
    """Test if the instances of AWSServerAccess are created properly."""
    instance = utils.get_mocked_bucket(class_=server_access.AWSServerAccess)
    assert instance.date_regex == re.compile(r'(\d{4}-\d{2}-\d{2}-\d{2}-\d{2}-\d{2})')
    assert instance.date_format == '%Y-%m-%d'

    mock_custom_bucket.assert_called_once()


@pytest.mark.parametrize('file_date, last_key_date, only_logs_after, is_old',
                         [(datetime(2019, 1, 1, 0, 0), datetime(2018, 1, 1, 0, 0), '20200101', True),
                          (datetime(2018, 1, 1, 0, 0), datetime(2020, 1, 1, 0, 0), '20190101', True),
                          (datetime(2018, 1, 1, 0, 0), None, '20190101', True),
                          (datetime(2020, 1, 1, 0, 0), datetime(2019, 1, 1, 0, 0), '20180101', False),
                          (datetime(2019, 1, 1, 0, 0), datetime(2018, 1, 1, 0, 0), None, False),
                          (None, datetime(2018, 1, 1, 0, 0), '20190101', False)
                          ])
def test_AWSServerAccess__key_is_old(only_logs_after, last_key_date, file_date, is_old):
    with patch('wazuh_integration.WazuhIntegration.get_sts_client'), \
            patch('wazuh_integration.WazuhIntegration.__init__'):
        instance = utils.get_mocked_bucket(class_=server_access.AWSServerAccess,
                                           only_logs_after=only_logs_after)

        assert instance._key_is_old(file_date, last_key_date) == is_old


@pytest.mark.parametrize('object_list', [utils.LIST_OBJECT_V2, utils.LIST_OBJECT_V2_NO_PREFIXES, utils.LIST_OBJECT_V2_TRUNCATED])
@pytest.mark.parametrize('reparse', [True, False])
@pytest.mark.parametrize('delete_file', [True, False])
@pytest.mark.parametrize('same_prefix_result', [True, False])
@patch('aws_bucket.aws_tools.debug')
@patch('aws_bucket.AWSBucket.build_s3_filter_args')
def test_AWSServerAccess_iter_files_in_bucket(mock_build_filter, mock_debug, same_prefix_result, delete_file, reparse, object_list):
    with patch('wazuh_integration.WazuhIntegration.get_sts_client'), \
            patch('wazuh_integration.WazuhIntegration.__init__'):

        instance = utils.get_mocked_bucket(class_=server_access.AWSServerAccess, bucket=utils.TEST_BUCKET,
                                           delete_file=delete_file, reparse=reparse,
                                           prefix=utils.TEST_PREFIX)

        mock_build_filter.return_value = {
            'Bucket': instance.bucket,
            'MaxKeys': 1000,
            'Prefix': utils.TEST_PREFIX
        }

        instance.client = MagicMock()
        instance.client.list_objects_v2.return_value = object_list

        aws_account_id = utils.TEST_ACCOUNT_ID
        aws_region = utils.TEST_REGION

        with patch('aws_bucket.AWSBucket._same_prefix', return_value=same_prefix_result) as mock_same_prefix, \
                patch('aws_bucket.AWSCustomBucket.already_processed', return_value=True) as mock_already_processed, \
                patch('aws_bucket.AWSBucket.get_log_file') as mock_get_log_file, \
                patch('aws_bucket.AWSBucket.iter_events') as mock_iter_events, \
                patch('aws_bucket.AWSCustomBucket.mark_complete') as mock_mark_complete:

            if 'IsTruncated' in object_list and object_list['IsTruncated']:
                instance.client.list_objects_v2.side_effect = [object_list, utils.LIST_OBJECT_V2_NO_PREFIXES]

            instance.iter_files_in_bucket(aws_account_id, aws_region)

            mock_build_filter.assert_any_call(aws_account_id, aws_region, custom_delimiter='-')
            instance.client.list_objects_v2.assert_called_with(**mock_build_filter(aws_account_id, aws_region))

            if 'Contents' not in object_list:
                mock_debug.assert_any_call(f"+++ No logs to process in bucket: {aws_account_id}/{aws_region}",
                                           1)
            else:
                for bucket_file in object_list['Contents']:
                    if not bucket_file['Key']:
                        continue

                    if bucket_file['Key'][-1] == '/':
                        continue

                    date_match = instance.date_regex.search(bucket_file['Key'])
                    match_start = date_match.span()[0] if date_match else None

                    mock_same_prefix.assert_called_with(match_start, aws_account_id, aws_region)

                    if not instance._same_prefix(match_start, aws_account_id, aws_region):
                        mock_debug.assert_any_call(f"++ Skipping file with another prefix: {bucket_file['Key']}", 2)
                        continue

                    mock_already_processed.assert_called_with(bucket_file['Key'], aws_account_id, aws_region)
                    if instance.reparse:
                        mock_debug.assert_any_call(
                            f"++ File previously processed, but reparse flag set: {bucket_file['Key']}",
                            1)
                    else:
                        mock_debug.assert_any_call(f"++ Skipping previously processed file: {bucket_file['Key']}", 2)
                        continue

                    mock_debug.assert_any_call(f"++ Found new log: {bucket_file['Key']}", 2)
                    mock_get_log_file.assert_called_with(aws_account_id, bucket_file['Key'])
                    mock_iter_events.assert_called()

                    if instance.delete_file:
                        mock_debug.assert_any_call(f"+++ Remove file from S3 Bucket:{bucket_file['Key']}", 2)

                    mock_mark_complete.assert_called_with(aws_account_id, aws_region, bucket_file)

                if object_list['IsTruncated']:
                    mock_build_filter.assert_any_call(aws_account_id, aws_region, True)



@patch('wazuh_integration.WazuhIntegration.get_sts_client')
@patch('wazuh_integration.WazuhIntegration.__init__')
def test_AWSConfigBucket_iter_files_in_bucket_ko(mock_integration, mock_sts):
    instance = utils.get_mocked_bucket(class_=server_access.AWSServerAccess)

    instance.client = MagicMock()

    with patch('aws_bucket.AWSBucket.build_s3_filter_args') as mock_build_filter:
        with pytest.raises(SystemExit) as e:
            instance.skip_on_error = False
            utils.LIST_OBJECT_V2_NO_PREFIXES['Contents'][0]['Key'] = ['123']
            instance.client.list_objects_v2.return_value = utils.LIST_OBJECT_V2_NO_PREFIXES
            instance.iter_files_in_bucket(utils.TEST_ACCOUNT_ID, utils.TEST_REGION)
        assert e.value.code == utils.INVALID_KEY_FORMAT_ERROR_CODE

        with pytest.raises(SystemExit) as e:
            mock_build_filter.side_effect = Exception
            instance.iter_files_in_bucket(utils.TEST_ACCOUNT_ID, utils.TEST_REGION)
        assert e.value.code == utils.UNEXPECTED_ERROR_WORKING_WITH_S3




@patch('wazuh_integration.WazuhIntegration.get_sts_client')
@patch('wazuh_integration.WazuhIntegration.__init__')
def test_AWSServerAccess_marker_only_logs_after(mock_integration, mock_sts):
    test_only_logs_after = utils.TEST_ONLY_LOGS_AFTER

    instance = utils.get_mocked_bucket(class_=server_access.AWSServerAccess, only_logs_after=test_only_logs_after)
    instance.prefix = utils.TEST_PREFIX

    instance.date_format = '%Y-%m-%d'

    marker = instance.marker_only_logs_after(aws_region=utils.TEST_REGION, aws_account_id=utils.TEST_ACCOUNT_ID)
    assert marker == f"{instance.prefix}{test_only_logs_after[0:4]}-{test_only_logs_after[4:6]}-{test_only_logs_after[6:8]}"


@patch('wazuh_integration.WazuhIntegration.get_sts_client')
@patch('wazuh_integration.WazuhIntegration.__init__')
def test_AWSServerAccess_check_bucket_ko_empty(mock_integration, mock_sts):
    instance = utils.get_mocked_bucket(class_=server_access.AWSServerAccess, bucket=utils.TEST_BUCKET)
    instance.client = MagicMock()
    instance.client.list_objects_v2.return_value = {'ResponseWithoutCommonPrefixes'}

    with pytest.raises(SystemExit) as e:
        instance.check_bucket()
    assert e.value.code == utils.EMPTY_BUCKET_ERROR_CODE


@pytest.mark.parametrize('error_code, exit_code', [
    (aws_bucket.THROTTLING_EXCEPTION_ERROR_CODE, utils.THROTTLING_ERROR_CODE),
    (aws_bucket.INVALID_CREDENTIALS_ERROR_CODE, utils.INVALID_CREDENTIALS_ERROR_CODE),
    (aws_bucket.INVALID_REQUEST_TIME_ERROR_CODE, utils.INVALID_REQUEST_TIME_ERROR_CODE),
    ("OtherClientError", utils.UNKNOWN_ERROR_CODE)
])
@patch('wazuh_integration.WazuhIntegration.get_sts_client')
@patch('wazuh_integration.WazuhIntegration.__init__')
def test_AWSServerAccess_check_bucket_ko_client(mock_integration, mock_sts,
                                         error_code, exit_code):
    instance = utils.get_mocked_bucket(class_=server_access.AWSServerAccess)
    instance.client = MagicMock()
    instance.client.list_objects_v2.side_effect = botocore.exceptions.ClientError({'Error': {'Code': error_code}},
                                                                                  "name")

    with pytest.raises(SystemExit) as e:
        instance.check_bucket()
    assert e.value.code == exit_code


@patch('aws_bucket.AWSCustomBucket.__init__')
def test_AWSServerAccess_load_information_from_file(mock_custom_bucket):
    instance = utils.get_mocked_bucket(class_=server_access.AWSServerAccess)

    data = 'bucket_owner test_bucket [29/Apr/2025:08:47:53 +0000] 0.0.0.0 arn:aws:iam::123456789123:user/test.user ' \
           'request_id operation - "GET /test_bucket?website= HTTP/1.1" 404 NoSuchWebsiteConfiguration 343 - 85 - "-" ' \
           '"S3Console/0.4, aws-internal/3 aws-sdk-java/1.11.991 Linux/4.9.230-0.1.ac.224.84.332.metal1.x86_64 ' \
           'OpenJDK_64-Bit_Server_VM/25.282-b08 java/1.8.0_282 vendor/Oracle_Corporation cfg/retry-mode/legacy" - ' \
           'host_id signature_version cipher_suite authentication_type s3.amazonaws.com TLSv1.2 '
    expected_information = [
        {"bucket_owner": "bucket_owner", "bucket": "test_bucket", "time": "29/Apr/2025:08:47:53 +0000",
         "remote_ip": "0.0.0.0", "requester": "arn:aws:iam::123456789123:user/test.user",
         "request_id": "request_id", "operation": "operation", "key": "-",
         "request_uri": "GET /test_bucket?website= HTTP/1.1", "http_status": "404",
         "error_code": "NoSuchWebsiteConfiguration", "bytes_sent": "343",
         "object_sent": "-", "total_time": "85",
         "turn_around_time": "-", "referer": "-",
         "user_agent": "S3Console/0.4, aws-internal/3 aws-sdk-java/1.11.991 Linux/4.9.230-0.1.ac.224.84.332.metal1.x86_64 OpenJDK_64-Bit_Server_VM/25.282-b08 java/1.8.0_282 vendor/Oracle_Corporation cfg/retry-mode/legacy",
         "version_id": "-", "host_id": "host_id",
         "signature_version": "signature_version", "cipher_suite": "cipher_suite",
         "authentication_type": "authentication_type", "host_header": "s3.amazonaws.com",
         "tls_version": "TLSv1.2", "source": "s3_server_access"}]

    with patch('aws_bucket.AWSBucket.decompress_file', mock_open(read_data=data)):
        assert instance.load_information_from_file(utils.TEST_LOG_KEY) == expected_information
