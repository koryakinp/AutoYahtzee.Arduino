#include <WString.h>

class StateManager {

  public:
    StateManager();
    
    void StepForward();
    String GetCurrentState();
    void ForceShutDownState();

  private:
    int _currentState;
    String _states[8];
};
