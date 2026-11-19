// Deterministic in-process transport for synthetic tests and the standalone demo.
#pragma once

#include "transport.h"

#define SWMP_MOCK_ENDPOINTS 16u
#define SWMP_MOCK_QUEUE_SIZE 16u

typedef struct SwmpMock SwmpMock;

typedef enum {
    SWMP_MOCK_NORMAL,
    SWMP_MOCK_DROP_STATE,
    SWMP_MOCK_DUPLICATE_STATE,
    SWMP_MOCK_REORDER_STATE  // queue before the newest pending state packet on the same link
} SwmpMockFault;

SwmpMock *swmp_mock_create(void);
void swmp_mock_destroy(SwmpMock *mock);
SwmpTransport swmp_mock_transport(SwmpMock *mock, unsigned endpoint);
void swmp_mock_fault(SwmpMock *mock, unsigned endpoint, SwmpMockFault next_state_send);
