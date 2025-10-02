"""
A simple python script to trigger a virtual alarm on
a Viewtron IP camera NVR. It does this by sending an HTTP Post 
to the API webhook endpoint on the NVR. 

Provided as a simple way to test the virtual alarm function on
Viewtron NVRs.

Written by Mike Haldas
mike@viewtron.com

Learn more about this project at
https://www.viewtron.com/valarm
"""

import http.client
import base64

# Configuration variables (modify these as needed)
API_IP = "192.168.0.147"  # NVR IP address or hostname
API_PORT = 80 # NVR HTTP port
API_URL = "/TriggerVirtualAlarm/17"  # Webhook API endpoint
API_USER = "admin"  # NVR username
API_PASSWORD = "my_password"  # NVR password

# XML message for virtual alarm API
API_XML = """
<?xml version="1.0" encoding="utf-8" ?>
<config version="2.0.0" xmlns="http://www.Sample.ipc.com/ver10">
	<action>
		<status>true</status>
	</action>
</config>
""" 

def send_api_post():
    try:
        # Establish connection
        conn = http.client.HTTPConnection(API_IP, API_PORT)

        # Prepare headers
        headers = {
            "Content-Type": "application/xml",
            "Accept": "application/xml",
            "Connection": "close"
        }

        # Add Basic Authentication header
        auth = base64.b64encode(f"{API_USER}:{API_PASSWORD}".encode()).decode()
        headers["Authorization"] = f"Basic {auth}"

        # If XML payload is provided, calculate Content-Length
        body = API_XML.encode('utf-8') if API_XML else b""
        if API_XML:
            headers["Content-Length"] = str(len(body))
        else:
            headers["Content-Length"] = "0"

        # Send POST request
        conn.request("POST", API_URL, body=body, headers=headers)

        # Get response
        response = conn.getresponse()
        status = response.status
        response_data = response.read().decode()

        # Check response status
        if status == 200:
            print("Request successful!")
            print("Response:", response_data)
        else:
            print(f"Request failed with status code {status}")
            print("Response:", response_data)

        # Close connection
        conn.close()

    except http.client.HTTPException as e:
        print(f"HTTP error occurred: {e}")
    except Exception as e:
        print(f"Error occurred: {e}")

if __name__ == "__main__":
    send_api_post()
