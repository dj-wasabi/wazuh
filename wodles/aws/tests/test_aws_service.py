import copy
import os
import sys
from datetime import datetime
from unittest.mock import MagicMock, patch

sys.path.append(os.path.join(os.path.dirname(os.path.realpath(__file__)), '.'))
import aws_utils as utils

sys.path.append(os.path.join(os.path.dirname(os.path.realpath(__file__)), '..'))
import wazuh_integration

sys.path.append(os.path.join(os.path.dirname(os.path.realpath(__file__)), '..', 'services'))
import aws_service

TEST_DATETIME = datetime.now()
TEST_DATETIME_STR = datetime.strftime(TEST_DATETIME, '%Y-%m-%dT%H:%M:%SZ')


@patch('wazuh_integration.WazuhIntegration.get_sts_client')
@patch('wazuh_integration.WazuhIntegration.__init__', side_effect=wazuh_integration.WazuhIntegration.__init__)
def test_AWSService__init__(mock_wazuh_integration, mock_sts):
    """Test if the instances of 'AWSService' are created properly."""
    mock_client = MagicMock()
    mock_sts.return_value = mock_client
    kwargs = utils.get_AWSService_parameters(db_table_name=utils.TEST_TABLE_NAME,
                                             service_name = utils.TEST_SERVICE_NAME, reparse=True,
                                             access_key=utils.TEST_ACCESS_KEY, secret_key=utils.TEST_SECRET_KEY,
                                             aws_profile=utils.TEST_AWS_PROFILE, iam_role_arn=utils.TEST_IAM_ROLE_ARN,
                                             only_logs_after=utils.TEST_ONLY_LOGS_AFTER, region=utils.TEST_REGION,
                                             discard_field=utils.TEST_DISCARD_FIELD,
                                             discard_regex=utils.TEST_DISCARD_REGEX,
                                             sts_endpoint=utils.TEST_STS_ENDPOINT,
                                             service_endpoint=utils.TEST_SERVICE_ENDPOINT,
                                             iam_role_duration=utils.TEST_IAM_ROLE_DURATION)
    instance = utils.get_mocked_service(**kwargs)
    mock_wazuh_integration.assert_called_with(instance, db_name=aws_service.DEFAULT_DATABASE_NAME,
                                              db_table_name=kwargs["db_table_name"],
                                              service_name=utils.TEST_SERVICE_NAME,
                                              access_key=kwargs["access_key"], secret_key=kwargs["secret_key"],
                                              aws_profile=kwargs["aws_profile"], iam_role_arn=kwargs["iam_role_arn"],
                                              region=kwargs["region"], discard_field=kwargs["discard_field"],
                                              discard_regex=kwargs["discard_regex"],
                                              sts_endpoint=kwargs["sts_endpoint"],
                                              service_endpoint=kwargs["service_endpoint"],
                                              iam_role_duration=kwargs["iam_role_duration"])
    assert instance.reparse == True
    assert instance.region == utils.TEST_REGION
    assert instance.only_logs_after == utils.TEST_ONLY_LOGS_AFTER
    mock_sts.assert_called_with(kwargs["access_key"], kwargs["secret_key"], kwargs["aws_profile"])
    mock_client.get_caller_identity.assert_called_once()


@patch('wazuh_integration.WazuhIntegration.get_sts_client')
@patch('wazuh_integration.WazuhIntegration.__init__', side_effect=wazuh_integration.WazuhIntegration.__init__)
def test_AWSService_get_last_log_date(mock_wazuh_integration, mock_sts):
    """Test 'get_last_log_date' function returns a date with the expected format."""
    instance = utils.get_mocked_service(only_logs_after=utils.TEST_ONLY_LOGS_AFTER)
    assert instance.get_last_log_date() == utils.TEST_ONLY_LOGS_AFTER_WITH_FORMAT


@patch('wazuh_integration.WazuhIntegration.get_sts_client')
@patch('wazuh_integration.WazuhIntegration.__init__', side_effect=wazuh_integration.WazuhIntegration.__init__)
def test_AWSService_format_message(mock_wazuh_integration, mock_sts):
    """Test 'format_message' function updates the expected fields of an event."""
    input_msg = {'service': 'service_name', 'createdAt': TEST_DATETIME, 'updatedAt': TEST_DATETIME, 'key': 'value'}
    output_msg = {'source': 'service_name', 'createdAt': TEST_DATETIME_STR, 'updatedAt': TEST_DATETIME_STR,
                  'key': 'value'}
    instance = utils.get_mocked_service(only_logs_after=utils.TEST_ONLY_LOGS_AFTER)
    expected_msg = copy.deepcopy(aws_service.AWS_SERVICE_MSG_TEMPLATE)
    expected_msg['aws'] = output_msg
    assert instance.format_message(input_msg) == expected_msg
