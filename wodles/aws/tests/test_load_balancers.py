import csv

import pytest
import os
import sys
from unittest.mock import patch, mock_open

sys.path.append(os.path.join(os.path.dirname(os.path.realpath(__file__)), '.'))
import aws_utils as utils

sys.path.append(os.path.join(os.path.dirname(os.path.realpath(__file__)), '..', 'buckets_s3'))
import aws_bucket
import load_balancers


@patch('aws_bucket.AWSCustomBucket.__init__')
def test_AWSLBBucket__init__(mock_custom_bucket):
    """Test if the instances of AWSLBBucket are created properly."""
    instance = utils.get_mocked_bucket(class_=load_balancers.AWSLBBucket)
    assert instance.service == 'elasticloadbalancing'
    mock_custom_bucket.assert_called_once()


@patch('wazuh_integration.WazuhIntegration.__init__')
@patch('wazuh_integration.WazuhIntegration.get_sts_client')
@patch('aws_bucket.AWSBucket.__init__', side_effect=aws_bucket.AWSBucket.__init__)
@patch('aws_bucket.AWSCustomBucket.__init__', side_effect=aws_bucket.AWSCustomBucket.__init__)
def test_AWSLBBucket_get_base_prefix(mock_custom_bucket, mock_bucket, mock_sts, mock_integration):
    instance = utils.get_mocked_bucket(class_=load_balancers.AWSLBBucket, prefix=f'{utils.TEST_PREFIX}/',
                                       suffix=f'{utils.TEST_SUFFIX}/')
    expected_base_prefix = f'{utils.TEST_PREFIX}/AWSLogs/{utils.TEST_SUFFIX}/'
    assert instance.get_base_prefix() == expected_base_prefix


@patch('load_balancers.AWSLBBucket.get_base_prefix', return_value='base_prefix/')
@patch('aws_bucket.AWSCustomBucket.__init__')
def test_AWSLBBucket_get_service_prefix(mock_custom_bucket, mock_base_prefix):
    instance = utils.get_mocked_bucket(class_=load_balancers.AWSLBBucket)
    expected_service_prefix = f'base_prefix/{utils.TEST_ACCOUNT_ID}/{instance.service}/'
    assert instance.get_service_prefix(utils.TEST_ACCOUNT_ID) == expected_service_prefix


@patch('aws_bucket.AWSBucket.iter_regions_and_accounts')
@patch('aws_bucket.AWSCustomBucket.__init__')
def test_AWSLBBucket_iter_regions_and_accounts(mock_custom_bucket, mock_iter_regions_accounts):
    instance = utils.get_mocked_bucket(class_=load_balancers.AWSLBBucket)
    instance.iter_regions_and_accounts(utils.TEST_ACCOUNT_ID, utils.TEST_REGION)

    mock_iter_regions_accounts.assert_called_with(instance, utils.TEST_ACCOUNT_ID, utils.TEST_REGION)


@patch('load_balancers.AWSLBBucket.get_service_prefix',
       return_value=f'base_prefix/{utils.TEST_ACCOUNT_ID}/elasticloadbalancing/')
@patch('aws_bucket.AWSCustomBucket.__init__')
def test_AWSLBBucket_get_full_prefix(mock_custom_bucket, mock_service_prefix):
    instance = utils.get_mocked_bucket(class_=load_balancers.AWSLBBucket)
    expected_full_prefix = f'base_prefix/{utils.TEST_ACCOUNT_ID}/elasticloadbalancing/{utils.TEST_REGION}/'
    assert instance.get_full_prefix(utils.TEST_ACCOUNT_ID, utils.TEST_REGION) == expected_full_prefix


@patch('aws_bucket.AWSBucket.mark_complete')
@patch('aws_bucket.AWSCustomBucket.__init__')
def test_AWSLBBucket_mark_complete(mock_custom_bucket, mock_mark_complete):
    test_log_file = 'log_file'

    instance = utils.get_mocked_bucket(class_=load_balancers.AWSLBBucket)
    instance.mark_complete(utils.TEST_ACCOUNT_ID, utils.TEST_REGION, test_log_file)

    mock_mark_complete.assert_called_with(instance, utils.TEST_ACCOUNT_ID, utils.TEST_REGION, test_log_file)


@patch('load_balancers.AWSLBBucket.__init__')
@patch('aws_bucket.AWSCustomBucket.__init__')
def test_AWSALBBucket__init__(mock_custom_bucket, mock_lb_bucket):
    """Test if the instances of AWSALBBucket are created properly."""
    instance = utils.get_mocked_bucket(class_=load_balancers.AWSALBBucket)
    mock_lb_bucket.assert_called_once()


@patch('load_balancers.AWSLBBucket.__init__')
@patch('aws_bucket.AWSCustomBucket.__init__')
def test_AWSALBBucket_load_information_from_file(mock_custom_bucket, mock_lb_bucket):
    instance = utils.get_mocked_bucket(class_=load_balancers.AWSALBBucket)

    with patch('aws_bucket.AWSBucket.decompress_file'), \
            patch('csv.DictReader') as mock_reader, \
            patch('aws_bucket.aws_tools.debug') as mock_debug:
        tsv_reader = [{
            'type': 'http', 'time': '2025-11-23T23:57:06.780380Z', 'elb': 'app/ALB/example',
            'client_port': '0.0.0.0:48888', 'target_port': '0.0.0.0:80', 'request_processing_time': '0.001',
            'target_processing_time': '0.001', 'response_processing_time': '0.000',
            'elb_status_code': '403', 'target_status_code': '403', 'received_bytes': '136',
            'sent_bytes': '5173', 'request': 'GET http://0.0.0.0:80/ HTTP/1.1',
            'user_agent': 'Mozilla/5.0 (compatible; Nimbostratus-Bot/v1.3.2; http://domain.com)',
            'ssl_cipher': '-', 'ssl_protocol': '-',
            'target_group_arn': 'arn:aws:elasticloadbalancing:region:123456789123:targetgroup/EC2/xxxxxxxxxxxxxx',
            'trace_id': 'Root=1-xxxxxxxx-xxxxxxxxxxxxxxxxxxxxxxxxxxxx', 'domain_name': '-',
            'chosen_cert_arn': '-', 'matched_rule_priority': '0',
            'request_creation_time': '2025-11-23T23:57:06.778000Z',
            'action_executed': 'forward', 'redirect_url': '-',
            'error_reason': '-', 'target_port_list': '0.0.0.0:80',
            'target_status_code_list': '403', 'classification': '-',
            'classification_reason': '-'
        }]

        mock_reader.return_value = tsv_reader

        expected_information = [{
            'type': 'http', 'time': '2025-11-23T23:57:06.780380Z', 'elb': 'app/ALB/example',
            'client_port': '48888', 'target_port': '80', 'client_ip': '0.0.0.0', 'target_ip': '0.0.0.0',
            'target_ip_list': '0.0.0.0', 'request_processing_time': '0.001', 'target_processing_time': '0.001',
            'response_processing_time': '0.000', 'elb_status_code': '403',
            'target_status_code': '403', 'received_bytes': '136', 'sent_bytes': '5173',
            'request': 'GET http://0.0.0.0:80/ HTTP/1.1',
            'user_agent': 'Mozilla/5.0 (compatible; Nimbostratus-Bot/v1.3.2; http://domain.com)',
            'ssl_cipher': '-', 'ssl_protocol': '-',
            'target_group_arn': 'arn:aws:elasticloadbalancing:region:123456789123:targetgroup/EC2/xxxxxxxxxxxxxx',
            'trace_id': 'Root=1-xxxxxxxx-xxxxxxxxxxxxxxxxxxxxxxxxxxxx', 'domain_name': '-',
            'chosen_cert_arn': '-', 'matched_rule_priority': '0',
            'request_creation_time': '2025-11-23T23:57:06.778000Z',
            'action_executed': 'forward', 'redirect_url': '-', 'error_reason': '-',
            'target_port_list': '80', 'target_status_code_list': '403',
            "source": "alb", 'classification': '-', 'classification_reason': '-'
        }]
        assert expected_information == instance.load_information_from_file(utils.TEST_LOG_KEY)

        # Force Error when handling IP:Port fields
        tsv_reader[0]['client_port'] = '0.0.0.0'
        instance.load_information_from_file(utils.TEST_LOG_KEY)
        mock_debug.assert_called()



@patch('load_balancers.AWSLBBucket.__init__')
@patch('aws_bucket.AWSCustomBucket.__init__')
def test_AWSCLBBucket__init__(mock_custom_bucket, mock_lb_bucket):
    """Test if the instances of AWSCLBBucket are created properly."""
    instance = utils.get_mocked_bucket(class_=load_balancers.AWSCLBBucket)
    mock_lb_bucket.assert_called_once()


@patch('load_balancers.AWSLBBucket.__init__')
@patch('aws_bucket.AWSCustomBucket.__init__')
def test_AWSCLBBucket_load_information_from_file(mock_custom_bucket, mock_lb_bucket):
    instance = utils.get_mocked_bucket(class_=load_balancers.AWSCLBBucket)

    with patch('aws_bucket.AWSBucket.decompress_file'), \
            patch('csv.DictReader') as mock_reader:
        mock_reader.return_value = [{
            "time": "2020-11-12T17:33:30.084203Z", "elb": "elb", "client_port": "0.0.0.0:55438",
            "backend_port": "0.0.0.0:55000", "request_processing_time": "0.000628",
            "backend_processing_time": "0.00001",
            "response_processing_time": "0.000015", "elb_status_code": "-",
            "backend_status_code": "-", "received_bytes": "1071",
            "sent_bytes": "2250", "request": "- - - ", "user_agent": "-",
            "ssl_cipher": "-", "ssl_protocol": "-"
        }]

        expected_information = [{
            "time": "2020-11-12T17:33:30.084203Z", "elb": "elb",
            "client_port": "0.0.0.0:55438", "backend_port": "0.0.0.0:55000",
            "request_processing_time": "0.000628", "backend_processing_time": "0.00001",
            "response_processing_time": "0.000015", "elb_status_code": "-",
            "backend_status_code": "-", "received_bytes": "1071",
            "sent_bytes": "2250", "request": "- - - ",
            "user_agent": "-", "ssl_cipher": "-",
            "ssl_protocol": "-", "source": "clb"
        }]

        assert expected_information == instance.load_information_from_file(utils.TEST_LOG_KEY)


@patch('load_balancers.AWSLBBucket.__init__')
@patch('aws_bucket.AWSCustomBucket.__init__')
def test_AWSNLBBucket___init__(mock_custom_bucket, mock_lb_bucket):
    """Test if the instances of AWSNLBBucket are created properly."""
    instance = utils.get_mocked_bucket(class_=load_balancers.AWSNLBBucket)
    mock_lb_bucket.assert_called_once()


@patch('load_balancers.AWSLBBucket.__init__')
@patch('aws_bucket.AWSCustomBucket.__init__')
def test_AWSNLBBucket_load_information_from_file(mock_custom_bucket, mock_lb_bucket):
    instance = utils.get_mocked_bucket(class_=load_balancers.AWSNLBBucket)

    with patch('aws_bucket.AWSBucket.decompress_file'), \
            patch('csv.DictReader') as mock_reader:
        tsv_reader = [{
            "type": "tls", "version": "2.0", "time": "2020-11-24T03:01:14",
            "elb": "elb", "listener": "listener", "client_port": "0.0.0.0:62215",
            "destination_port": "0.0.0.0:55000", "connection_time": "17553",
            "tls_handshake_time": "-", "received_bytes": "0", "sent_bytes": "0",
            "incoming_tls_alert": "-", "chosen_cert_arn": "-",
            "chosen_cert_serial": "-", "tls_cipher": "-", "tls_protocol_version": "-",
            "tls_named_group": "-", "domain_name": "-",
            "alpn_fe_protocol": "-", "alpn_client_preference_list": "-", "null": "-"
        }]

        mock_reader.return_value = tsv_reader

        expected_information = [{
            "type": "tls", "version": "2.0", "time": "2020-11-24T03:01:14",
            "elb": "elb", "listener": "listener",
            "client_port": "62215", "destination_port": "55000",
            "connection_time": "17553", "tls_handshake_time": "-",
            "received_bytes": "0", "sent_bytes": "0", "incoming_tls_alert": "-",
            "chosen_cert_arn": "-", "chosen_cert_serial": "-",
            "tls_cipher": "-", "tls_protocol_version": "-",
            "tls_named_group": "-", "domain_name": "-",
            "alpn_fe_protocol": "-", "alpn_client_preference_list": "-",
            "null": "-", "source": "nlb", "client_ip": "0.0.0.0", "destination_ip": "0.0.0.0"
        }]

        assert expected_information == instance.load_information_from_file(utils.TEST_LOG_KEY)

        # Force Error when handling IP:Port fields
        tsv_reader[0]['client_port'] = '0.0.0.0'
        tsv_reader[0]['destination_port'] = '0.0.0.0'
        result = instance.load_information_from_file(utils.TEST_LOG_KEY)
        assert result[0]['client_ip'] == tsv_reader[0]['client_port']
        assert result[0]['destination_ip'] == tsv_reader[0]['destination_port']