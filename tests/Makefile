# =============================================================================
#  CANM8 — Tests unitaires host (gcc/clang, macOS/Linux)
#  Cible : logique pure (ring buffer, learner, vehicle_id, eeprom, obd2)
#  Aucune dépendance externe — pas de XC8, pas de PIC18, pas de framework
# =============================================================================

CC      = gcc
CFLAGS  = -std=c99 -Wall -Wextra -Wpedantic \
          -I. -I.. \
          -DTEST_BUILD \
          -D_XTAL_FREQ=24000000UL \
          -include xc_stub.h \
          -Wno-unused-parameter \
          -Wno-unused-function

# Objet stub commun à tous les exécutables
STUB    = xc_stub.o

TESTS   = test_can test_vehicle_id test_eeprom test_obd2 test_learner

.PHONY: all clean run

all: $(TESTS)

$(STUB): xc_stub.c xc_stub.h
	$(CC) $(CFLAGS) -c xc_stub.c -o $@

# Chaque test inclut directement le .c source via #include "../xxx.c"
# → compilation en une seule unité, pas besoin de linker les objets séparément
test_can: test_can.c $(STUB)
	$(CC) $(CFLAGS) $< $(STUB) -o $@

test_vehicle_id: test_vehicle_id.c $(STUB)
	$(CC) $(CFLAGS) $< $(STUB) -o $@

test_eeprom: test_eeprom.c $(STUB)
	$(CC) $(CFLAGS) $< $(STUB) -o $@

test_obd2: test_obd2.c $(STUB)
	$(CC) $(CFLAGS) $< $(STUB) -o $@

test_learner: test_learner.c $(STUB)
	$(CC) $(CFLAGS) $< $(STUB) -o $@

run: all
	@echo ""
	@failed=0; \
	for t in $(TESTS); do \
	    echo "──────────────────────────────────────────"; \
	    ./$$t || failed=$$((failed+1)); \
	done; \
	echo ""; \
	echo "══════════════════════════════════════════"; \
	if [ $$failed -eq 0 ]; then \
	    echo "  ALL TEST SUITES PASSED"; \
	else \
	    echo "  $$failed SUITE(S) FAILED"; \
	fi; \
	echo "══════════════════════════════════════════"; \
	exit $$failed

clean:
	rm -f $(TESTS) $(STUB)
