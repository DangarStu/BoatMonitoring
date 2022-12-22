// Class to represent a battery or battery bank.

class Battery {
private:
  float nominalVoltage;
  float minimumVoltage;
  float maximumVoltage;
  float currentVoltage;

public:
  // Constructor
  Battery(float nominalVoltage, float minimumVoltage, float maximumVoltage, float currentVoltage)
      : nominalVoltage(nominalVoltage),
        minimumVoltage(minimumVoltage),
        maximumVoltage(maximumVoltage),
        currentVoltage(currentVoltage) {}

  // Getters
  float getNominalVoltage() const { return nominalVoltage; }
  float getMinimumVoltage() const { return minimumVoltage; }
  float getMaximumVoltage() const { return maximumVoltage; }
  float getCurrentVoltage() const { return currentVoltage; }

  // Setters
  void setNominalVoltage(float voltage) { nominalVoltage = voltage; }
  void setMinimumVoltage(float voltage) { minimumVoltage = voltage; }
  void setMaximumVoltage(float voltage) { maximumVoltage = voltage; }
  void setCurrentVoltage(float voltage) { currentVoltage = voltage; }
};