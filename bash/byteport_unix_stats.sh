#!/bin/bash
#  www.byteport.se
#    Simple example client using cURL (curl.haxx.se)
#
########################################################

CURL_BASE_STANDARD="curl --silent"
CURL_BASE_SOCKS5="curl --silent --socks5 localhost:5000"
CURL_BASE="$CURL_BASE_STANDARD"
NAMESPACE=$1
API_KEY=$2
RATE=60
BYTEPORT_API_HOST="api.byteport.se"

########################################################
# Check for binaries
hash curl 2>/dev/null || { echo >&2 "This program require cURL but it's not installed. Aborting."; exit 1; }

if [ -n "$3" ]; then
    DEVICE_UID=$3
else
    DEVICE_UID=`hostname| tr . _`
fi

BYTEPORT_BASE_URL="http://$BYTEPORT_API_HOST/services/store/$NAMESPACE/$DEVICE_UID/?_key=$API_KEY"

if [ "$#" -lt 2 ]; then
    echo "Usage: $0 [namespace] [namespace api write key] <device_uid (optional)>"
    echo ""
    echo "This script will collect a number of common paramters useful for continous logging of a UNIX system"
    echo ""
    echo "The script employs curl or optionally wget to send data to $BYTEPORT_API_HOST"
    echo
    exit 1
fi
echo "Namespace  = $NAMESPACE"
echo "Device UID = $DEVICE_UID"
echo "Base URL = $BYTEPORT_BASE_URL"

# Test base first
response_code=`$CURL_BASE -s -o /dev/null -w "%{http_code}" $BYTEPORT_BASE_URL`

if [ "$response_code" == "200" ]; then
    echo "Byteport API test call OK!"
fi

if [ "$response_code" == "403" ]; then
    echo ""
    echo "Error: Byteport responded with "HTTP 403: Forbidden", verify that"
    echo "- The namepspace $NAMESPACE does allow write by HTTP GET and that you supplied the right API key to this script"
    echo "    go to http://www.byteport.se/manager/namespace/security/$NAMESPACE/ to configure this."
    echo ""
    exit $response_code
fi

if [ "$response_code" == "404" ]; then
    echo ""
    echo "Error: Byteport responded with "HTTP 404: Not found", verify that"
    echo "- $NAMESPACE is valid namespace and open for access"
    echo "- $DEVICE_UID is a registered device under namespace $NAMESPACE"
    echo "   To register, go to http://www.byteport.se/manager/devices/register/$NAMESPACE/?uid=$DEVICE_UID"
    echo ""
    exit $response_code
fi

while :
do	
	rdv=`df -k / | awk '$3 ~ /[0-9]+/ { print $4 }'`
	la5v=`uptime | awk '{ print $9 }'| tr -d ','`
    est_ports=`netstat -ant | awk '{print $6}' | sort | uniq -c | sort -n |tail -1|awk '{print $1}'`

    data_string="rd_free=$rdv&la5=$la5v&est_ports=$est_ports"

    if [ -d "/sys/class/net/wlan0/" ]; then
        wlan0_rx_mb=$((`cat /sys/class/net/wlan0/statistics/rx_bytes`/1024/1024))
        wlan0_tx_mb=$((`cat /sys/class/net/wlan0/statistics/tx_bytes`/1024/1024))
        wlan0_level=$((`grep wlan0 /proc/net/wireless|awk '{ print \$4 }'|sed 's/\.$//'`))
        wlan0_data="wlan0_rx_mb=$wlan0_rx_mb&wlan0_tx_mb=$wlan0_tx_mb&wlan0_level=$wlan0_level"
        data_string="$data_string&$wlan0_data"
        if [ -f "/sbin/wpa_cli" ]; then
            ssid=`wpa_cli status|grep ssid|grep -v bssid`
            bssid=`wpa_cli status|grep bssid`
            data_string="$data_string&$ssid&$bssid"
        fi
    fi
    if [ -d "/sys/class/net/eth0/" ]; then
        eth0_rx_mb=$((`cat /sys/class/net/eth0/statistics/rx_bytes`/1024/1024))
        eth0_tx_mb=$((`cat /sys/class/net/eth0/statistics/tx_bytes`/1024/1024))
        eth0_data="eth0_rx_mb=$eth0_rx_mb&eth0_tx_mb=$eth0_tx_mb"
        data_string="$data_string&$eth0_data"
    fi
	# Replace whitespace with %20
	data_string=`echo $data_string| sed 's/[[:space:]]/%20/g'`

	$CURL_BASE -s "$BYTEPORT_BASE_URL&$data_string"

	sleep $RATE
done
