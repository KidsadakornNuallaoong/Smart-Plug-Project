const float Reference = 5.0;
const float Sentity = 0.1;
const float Calibration_Fac = 1.0;

float Get_Ampare(float Fate_pin){
  float voltage = Fate_pin * Reference / 1023.0;
  return (voltage - Reference/2) / Sentity * Calibration_Fac;
}

float Get_Watt(float Crr){
  return Crr * Reference;
}

float Get_WKH(float Power){
  return Power / 1000.0 / 60.0 / 60.0;
}

float Currency_THB(float Wkh){
  return Wkh * 3.5;
}