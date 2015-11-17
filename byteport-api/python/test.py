from byteport.clients import ByteportHttpClient

client = ByteportHttpClient('test', 'd8a26587463268f88fea6aec', 'barDev1')

# NOTE: This will block the current thread!
client.poll_directory_and_store_upon_content_change('/home/iot_user/measured_values/', 'barDev1', poll_interval=10)
