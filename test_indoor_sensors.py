#!/usr/bin/env python3
"""
Indoor Sensor API Test Script

This script tests the indoor sensor functionality by simulating sensor data
submissions to the attic fan controller's REST API.

Usage:
    python3 test_indoor_sensors.py [controller_ip]

Example:
    python3 test_indoor_sensors.py 192.168.1.100
"""

import requests
import json
import time
import random
import sys

# Default controller IP (update as needed)
CONTROLLER_IP = sys.argv[1] if len(sys.argv) > 1 else "192.168.1.100"
BASE_URL = f"http://{CONTROLLER_IP}"

# Test sensor configurations
TEST_SENSORS = [
    {"sensorId": "test_living_room", "name": "Living Room", "base_temp": 72.0, "base_humidity": 45.0},
    {"sensorId": "test_bedroom", "name": "Master Bedroom", "base_temp": 70.0, "base_humidity": 50.0},
    {"sensorId": "test_kitchen", "name": "Kitchen", "base_temp": 74.0, "base_humidity": 42.0}
]

def test_sensor_registration():
    """Test registering multiple sensors with different data"""
    print(f"🧪 Testing sensor registration on {CONTROLLER_IP}")
    
    for sensor in TEST_SENSORS:
        # Add some random variation to the sensor data
        temperature = sensor["base_temp"] + random.uniform(-2.0, 2.0)
        humidity = sensor["base_humidity"] + random.uniform(-5.0, 5.0)
        
        data = {
            "sensorId": sensor["sensorId"],
            "name": sensor["name"],
            "temperature": round(temperature, 1),
            "humidity": round(humidity, 1)
        }
        
        try:
            response = requests.post(f"{BASE_URL}/indoor_sensors/data", 
                                   json=data, 
                                   timeout=5)
            
            if response.status_code == 200:
                print(f"✅ {sensor['name']}: {temperature:.1f}°F, {humidity:.1f}% RH")
            else:
                print(f"❌ {sensor['name']}: HTTP {response.status_code} - {response.text}")
                
        except requests.exceptions.RequestException as e:
            print(f"❌ {sensor['name']}: Connection error - {e}")
    
    print()

def test_get_sensors():
    """Test retrieving all sensor data"""
    print("📊 Testing sensor data retrieval")
    
    try:
        response = requests.get(f"{BASE_URL}/indoor_sensors", timeout=5)
        
        if response.status_code == 200:
            data = response.json()
            print(f"✅ Found {data['count']} active sensors (max: {data['maxSensors']})")
            
            if data['averageTemperature'] and data['averageHumidity']:
                print(f"📈 Average: {data['averageTemperature']}°F, {data['averageHumidity']}% RH")
            
            print("\n🏠 Individual sensors:")
            for sensor in data['sensors']:
                age = sensor['secondsSinceUpdate']
                print(f"   • {sensor['name']}: {sensor['temperature']}°F, {sensor['humidity']}% RH "
                      f"(updated {age}s ago)")
        else:
            print(f"❌ HTTP {response.status_code} - {response.text}")
            
    except requests.exceptions.RequestException as e:
        print(f"❌ Connection error - {e}")
    
    print()

def test_controller_status():
    """Test that indoor sensor data appears in main status"""
    print("🏠 Testing main controller status integration")
    
    try:
        response = requests.get(f"{BASE_URL}/status", timeout=5)
        
        if response.status_code == 200:
            data = response.json()
            
            if data.get('indoorSensorsEnabled'):
                count = data.get('indoorSensorCount', 0)
                avg_temp = data.get('avgIndoorTemp')
                avg_humidity = data.get('avgIndoorHumidity')
                
                print(f"✅ Indoor sensors enabled: {count} active sensors")
                if avg_temp and avg_humidity:
                    print(f"📊 Status shows: {avg_temp}°F, {avg_humidity}% RH average")
                else:
                    print("⚠️ No average data available yet")
            else:
                print("❌ Indoor sensors are disabled in controller config")
        else:
            print(f"❌ HTTP {response.status_code} - {response.text}")
            
    except requests.exceptions.RequestException as e:
        print(f"❌ Connection error - {e}")
    
    print()

def test_sensor_removal():
    """Test removing a sensor"""
    print("🗑️ Testing sensor removal")
    
    test_sensor_id = TEST_SENSORS[0]["sensorId"]
    
    try:
        response = requests.delete(f"{BASE_URL}/indoor_sensors/{test_sensor_id}", timeout=5)
        
        if response.status_code == 200:
            print(f"✅ Successfully removed sensor: {test_sensor_id}")
        elif response.status_code == 404:
            print(f"ℹ️ Sensor not found (already removed): {test_sensor_id}")
        else:
            print(f"❌ HTTP {response.status_code} - {response.text}")
            
    except requests.exceptions.RequestException as e:
        print(f"❌ Connection error - {e}")
    
    print()

def main():
    print("🌀 Attic Fan Controller - Indoor Sensor API Test")
    print("=" * 50)
    print(f"Controller: {BASE_URL}")
    print()
    
    # Run tests in sequence
    test_sensor_registration()
    time.sleep(2)  # Allow time for data to be processed
    
    test_get_sensors()
    test_controller_status()
    test_sensor_removal()
    
    # Verify removal worked
    test_get_sensors()
    
    print("🎉 Test complete!")
    print("\n💡 Tips:")
    print("   • Check the controller's web UI for sensor data")
    print("   • Monitor the diagnostics log for sensor registration events")
    print("   • Enable MQTT to see sensor data in Home Assistant")

if __name__ == "__main__":
    main()