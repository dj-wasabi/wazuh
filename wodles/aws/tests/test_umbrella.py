import pytest
import os
import sys
from unittest.mock import patch, mock_open

sys.path.append(os.path.join(os.path.dirname(os.path.realpath(__file__)), '.'))
import aws_utils as utils

sys.path.append(os.path.join(os.path.dirname(os.path.realpath(__file__)), '..', 'buckets_s3'))
import aws_bucket
import umbrella


@patch('aws_bucket.AWSCustomBucket.__init__')
def test_CiscoUmbrella__init__(mock_custom_bucket):
    """Test if the instances of CiscoUmbrella are created properly."""
    instance = utils.get_mocked_bucket(class_=umbrella.CiscoUmbrella)
    assert not instance.check_prefix
    assert instance.date_format == '%Y-%m-%d'

    mock_custom_bucket.assert_called_once()


@pytest.mark.parametrize('prefix, data, expected_result', [
    ('dnslogs',
     '"2015-01-16 17:48:41","ActiveDirectoryUserName", "ActiveDirectoryUserName,ADSite,Network", "0.0.0.0","0.0.0.0",'
     '"Allowed","1 (A)", "NOERROR","domain-visited.com.", "Chat,Photo Sharing,Social Networking,Allow List"',
     [{"timestamp": "2015-01-16 17:48:41", "most_granular_identity": "ActiveDirectoryUserName",
       "identities": " \"ActiveDirectoryUserName", "internal_ip": "ADSite", "external_ip": "Network\"",
       "action": " \"0.0.0.0\"", "query_type": "0.0.0.0", "response_code": "Allowed", "domain": "1 (A)",
       "categories": " \"NOERROR\"", "most_granular_identity_type": "domain-visited.com.",
       "identity_types": " \"Chat", "blocked_categories": "Photo Sharing",
       None: [
           "Social Networking",
           "Allow List\""]}
      ]),
    ('proxylogs',
     '"2017-10-02 23:52:53","TheComputerName","ActiveDirectoryUserName, ADSite,Network","0.0.0.0","0.0.0.0","",'
     '"ALLOWED", "http://example.com/the.js","www.example.com", "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_11_6) '
     'AppleWebKit/537.36 (KHTML, like Gecko) Chrome/61.0.3163.100 Safari/537.36","200", "562","1489","","","","","",'
     '"","","Networks"',
     [{"timestamp": "2017-10-02 23:52:53", "identities": "TheComputerName",
       "internal_ip": "ActiveDirectoryUserName, ADSite,Network",
       "external_ip": "0.0.0.0", "destination_ip": "0.0.0.0", "content_type": "", "verdict": "ALLOWED",
       "url": " \"http://example.com/the.js\"",
       "referer": "www.example.com",
       "user_agent": " \"Mozilla/5.0 (Macintosh; Intel Mac OS X 10_11_6) AppleWebKit/537.36 (KHTML",
       "status_code": " like Gecko) Chrome/61.0.3163.100 Safari/537.36\"", "requested_size": "200",
       "response_size": " \"562\"", "response_body_size": "1489", "sha": "", "categories": "", "av_detections": "",
       "puas": "",
       "amp_disposition": "", "amp_malware_name": "", "amp_score": "", "identity_type": "Networks"}]),
    ('iplogs',
     '"2017-10-02 19:58:12","TheComputerName","0.0.0.0", "55605","0.0.0.0","443","Unauthorized IP Tunnel Access"',
     [{"timestamp": "2017-10-02 19:58:12",
       "identity": "TheComputerName",
       "source_ip": "0.0.0.0",
       "source_port": " \"55605\"",
       "destination_ip": "0.0.0.0",
       "destination_port": "443",
       "categories": "Unauthorized IP Tunnel Access"}])
])
@patch('aws_bucket.AWSCustomBucket.__init__')
def test_CiscoUmbrella_load_information_from_file(mock_custom_bucket, prefix, data, expected_result):
    instance = utils.get_mocked_bucket(class_=umbrella.CiscoUmbrella)
    instance.prefix = prefix

    expected_result[0].update({'source': 'cisco_umbrella'})

    with patch('aws_bucket.AWSBucket.decompress_file', mock_open(read_data=data)):
        assert instance.load_information_from_file(utils.TEST_LOG_KEY) == expected_result


@patch('aws_bucket.AWSCustomBucket.__init__')
def test_CiscoUmbrella_load_information_from_file_ko(mock_custom_bucket):
    instance = utils.get_mocked_bucket(class_=umbrella.CiscoUmbrella)
    instance.prefix = 'Error prefix'

    with patch('aws_bucket.AWSBucket.decompress_file'), \
            pytest.raises(SystemExit) as e:
        instance.load_information_from_file(utils.TEST_LOG_KEY)
    assert e.value.code == utils.INVALID_TYPE_ERROR_CODE


@patch('wazuh_integration.WazuhIntegration.get_sts_client')
@patch('wazuh_integration.WazuhIntegration.__init__')
@patch('aws_bucket.AWSCustomBucket.__init__', side_effect=aws_bucket.AWSBucket.__init__)
def test_CiscoUmbrella_marker_only_logs_after(mock_custom_bucket, mock_integration, mock_sts):
    test_only_logs_after = utils.TEST_ONLY_LOGS_AFTER

    instance = utils.get_mocked_bucket(class_=umbrella.CiscoUmbrella, only_logs_after=test_only_logs_after)
    instance.prefix = utils.TEST_PREFIX

    instance.date_format = '%Y-%m-%d'

    marker = instance.marker_only_logs_after(aws_region=utils.TEST_REGION, aws_account_id=utils.TEST_ACCOUNT_ID)
    assert marker == f"{instance.prefix}{test_only_logs_after[0:4]}-{test_only_logs_after[4:6]}-{test_only_logs_after[6:8]}"
