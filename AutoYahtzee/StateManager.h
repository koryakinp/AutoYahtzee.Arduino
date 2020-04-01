#include <WString.h>

class StateManager {

  public:
    StateManager();
    
    void StepForward();
    String GetCurrentState();

  private:
    int _currentState;
    String _states[7];
};
