#include "StateManager.h"
#include "States.h"

StateManager::StateManager() {
  _currentState = 0;

  _states[0] = States::UNLOAD;
  _states[1] = States::PRE_LIFT;
  _states[2] = States::LIFT;
  _states[3] = States::THROW;
  _states[4] = States::PRE_LOWER; 
  _states[5] = States::LOWER;
  _states[6] = States::PRE_UNLOAD;  
}
    
void StateManager::StepForward() {
  _currentState++;
  if(_currentState == 7) {
    _currentState = 0;
  }
}

String StateManager::GetCurrentState() {
  return _states[_currentState];
}
