// Copyright (C) 2026 Piers Finlayson <piers@piers.rocks>
//
// MIT License

// epio - A PIO emulator
//
// Unit tests for epio_force_sm_instr()

#define APIO_LOG_IMPL
#include "test.h"

static void forced_jmp_sets_pc(void **state) {
    (void)state;
    epio_t *epio = epio_init();
    assert_non_null(epio);

    epio_force_sm_instr(epio, 0, 0, APIO_JMP(5));
    assert_int_equal(epio_peek_sm_pc(epio, 0, 0), 5);
    assert_false(epio_is_sm_enabled(epio, 0, 0));

    epio_free(epio);
}

static void forced_set_keeps_pc(void **state) {
    (void)state;
    epio_t *epio = epio_init();
    assert_non_null(epio);

    epio_force_sm_instr(epio, 0, 0, APIO_JMP(3));
    epio_force_sm_instr(epio, 0, 0, APIO_SET_X(7));
    assert_int_equal(epio_peek_sm_x(epio, 0, 0), 7);
    assert_int_equal(epio_peek_sm_pc(epio, 0, 0), 3);

    epio_free(epio);
}

static void forced_delay_ignored(void **state) {
    (void)state;
    epio_t *epio = epio_init();
    assert_non_null(epio);

    epio_force_sm_instr(epio, 0, 0, APIO_ADD_DELAY(APIO_SET_X(1), 5));
    assert_int_equal(epio_peek_sm_x(epio, 0, 0), 1);
    assert_int_equal(epio_peek_sm_delay(epio, 0, 0), 0);

    epio_free(epio);
}

static void pending_delay_kept(void **state) {
    (void)state;
    epio_t *epio = epio_init();
    assert_non_null(epio);

    epio_set_instr(epio, 0, 0, APIO_ADD_DELAY(APIO_NOP, 3));
    epio_enable_sm(epio, 0, 0);
    epio_step_cycles(epio, 1);
    assert_int_equal(epio_peek_sm_delay(epio, 0, 0), 3);

    epio_force_sm_instr(epio, 0, 0, APIO_SET_X(7));
    assert_int_equal(epio_peek_sm_x(epio, 0, 0), 7);
    assert_int_equal(epio_peek_sm_delay(epio, 0, 0), 3);

    epio_step_cycles(epio, 3);
    assert_int_equal(epio_peek_sm_delay(epio, 0, 0), 0);

    epio_free(epio);
}

// A forced instruction that has to wait, for example a WAIT on a pin that is
// low, is skipped.  On real hardware, the state machine waits until the
// instruction completes.  The SM runs the instruction at its PC next.
static void forced_stall_dropped(void **state) {
    (void)state;
    epio_t *epio = epio_init();
    assert_non_null(epio);

    epio_set_instr(epio, 0, 0, APIO_SET_Y(20));
    epio_force_sm_instr(epio, 0, 0, APIO_PULL_BLOCK);
    assert_int_equal(epio_peek_sm_pc(epio, 0, 0), 0);

    epio_push_tx_fifo(epio, 0, 0, 0xDEADBEEF);
    epio_enable_sm(epio, 0, 0);
    epio_step_cycles(epio, 1);
    assert_int_equal(epio_peek_sm_y(epio, 0, 0), 20);
    assert_int_equal(epio_peek_sm_osr(epio, 0, 0), 0);
    assert_int_equal(epio_tx_fifo_depth(epio, 0, 0), 1);

    epio_free(epio);
}

static void force_invalid_block_sm(void **state) {
    (void)state;
    epio_t *epio = epio_init();
    assert_non_null(epio);

    expect_assert_failure(epio_force_sm_instr(epio, NUM_PIO_BLOCKS, 0, APIO_NOP));
    expect_assert_failure(epio_force_sm_instr(epio, 0, NUM_SMS_PER_BLOCK, APIO_NOP));

    epio_free(epio);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(forced_jmp_sets_pc),
        cmocka_unit_test(forced_set_keeps_pc),
        cmocka_unit_test(forced_delay_ignored),
        cmocka_unit_test(pending_delay_kept),
        cmocka_unit_test(forced_stall_dropped),
        cmocka_unit_test(force_invalid_block_sm),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
