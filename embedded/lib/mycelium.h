#ifndef MYCELIUM_H
#define MYCELIUM_H

enum m6m_state_t {
  M6M_STATE_ABSENT,
  M6M_STATE_DEFERRING,
  M6M_STATE_DISABLED,
  M6M_STATE_ENABLED,
  M6M_STATE_IN_TEST,
  M6M_STATE_QUALIFIED,
  M6M_STATE_QUIESCED,
  M6M_STATE_STANDBY_OFFLINE,
  M6M_STATE_STANDBY_SPARE,
  M6M_STATE_STARTING,
  M6M_STATE_UNAVAILABLE_OFFLINE,
  M6M_STATE_UPDATING,
};

#endif