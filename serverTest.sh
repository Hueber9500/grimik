curl -H Content-Type:application/json -X POST http://localhost:12345/register -d '{"name":"hueber", "password": "111"}'
curl -H Content-Type:application/json -X POST http://localhost:12345/login -d '{"name":"hueber", "password": "111"}'
curl -H Content-Type:application/json -X POST http://localhost:12345/usr_data -d @/home/mario/Projects/c++/test.json


