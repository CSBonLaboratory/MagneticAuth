#include <InfluxDbClient.h>

int userExists(int id);

void writeFluxPoint(int val);

void initTimeSeries(int id, int flag, unsigned long startTimeMillis);

int connectToBuckets();
