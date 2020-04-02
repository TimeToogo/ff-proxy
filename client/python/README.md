FF Proxy - Python Client
========================

FF includes a client library for use in Python projects.

## Installation

The python package `ff-client` can be installed with pip:

```
pip install ff-client
```

See latest versions [here](https://pypi.org/project/ff-client/)

## Usage

The `FfClient` class provides the ability to send a HTTP request string to an FF proxy:

```python
from ff_client import FfClient, FfConfig, FfRequest
import logging

client = FfClient(
    FfConfig(
        ip_address='127.0.0.1',
        port=8080, 
        pre_shared_key='testkey',
        log_level=logging.DEBUG
    )
)

client.send_request(
'''
GET / HTTP/1.1
Host: google.com

''',
https=True
)
```
