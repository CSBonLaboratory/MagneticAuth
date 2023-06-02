#include "magnetic_influx.h"

#define INFLUXDB_URL "http://10.41.248.112:8086"

#define INFLUXDB_TOKEN "OozhZ4q3BGjkup_KWbpTBjgen1UlAvhl5dyPnqweiXbg2fPBhb41F3pJV8_3HEZEgfEVKT-rq_9mrPPFICja3g=="

#define INFLUXDB_ORG "upb"

#define BUCKET_USERS "Signatures"

#define BUCKET_ATTEMPTS "MagneticAttempts"

#define MEASUREMENT "magnetic"

#define USERS_INDEX 0

#define ATTEMPTS_INDEX 1

#define MAX_BATCH_SIZE 10

#define WRITE_BUFFER_SIZE 30

unsigned long startTimeSerieMillis = 0;

int currentId = -1;

int currentBucketIdx = -1;

Point hallPoint(MEASUREMENT);

InfluxDBClient clients[] = {
  InfluxDBClient(INFLUXDB_URL, INFLUXDB_ORG, BUCKET_USERS, INFLUXDB_TOKEN),
  InfluxDBClient(INFLUXDB_URL, INFLUXDB_ORG, BUCKET_ATTEMPTS, INFLUXDB_TOKEN)
  };

void initTimeSeries(int id,int flag, unsigned long startTimeMillis){
  hallPoint.clearTags();
  startTimeSerieMillis = startTimeMillis;
  currentId = id;
  currentBucketIdx = flag;
  hallPoint.addTag("id", String(id));
}

void writeFluxPoint(int val){
  hallPoint.addField("sign", val);
  //time_t tnow = time(nullptr);
  //hallPoint.setTime(tnow);
  if(!clients[currentBucketIdx].writePoint(hallPoint)){
    Serial.println(clients[currentBucketIdx].getLastErrorMessage());
  }
  
  hallPoint.clearFields();
}

int userExists(int id){
  String existQuery = String("from(bucket: \"") + String(BUCKET_USERS) + String("\")");
  existQuery += String("|> range(start: 2021-05-22T23:30:00Z)");
  existQuery += String("|> filter(fn: (r) => r._measurement == \"") + String(MEASUREMENT) + String("\" and r.id == \"") + String(id) + String("\")"); 
  existQuery += String("|> count()");

  Serial.println(existQuery);
  int userFound = 0;
  FluxQueryResult result = clients[USERS_INDEX].query(existQuery);

  while(result.next()){
    if(result.getValueByName("_value").getLong() > 0){
      Serial.println("User was registered in the past");
      userFound = 1;
    }
  }

  if(result.getError() != ""){
    Serial.print("Query result error: ");
    Serial.println(result.getError());
    return -1;
  }

  Serial.print("User id");
  Serial.println(id);
  Serial.println(" is available.");
  result.close();
  return userFound;

}

int connectToBuckets(){
  
  for(int i = 0; i < 2; i++){
    if (clients[i].validateConnection()) {
        Serial.print("Connected to InfluxDB client : ");
        Serial.print(i);
        Serial.println(clients[i].getServerUrl());
        //clients[i].setWriteOptions(WriteOptions().writePrecision(WritePrecision::MS));
      } else {
        Serial.print("InfluxDB connection failed : ");
        Serial.print(i);
        Serial.println(clients[i].getLastErrorMessage());
        return -1;
      }
  }
  return 0;

}
