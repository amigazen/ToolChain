/* Test __interrupt keyword */

__interrupt void interrupt_handler() {
    /* This function should be marked as an interrupt handler */
    /* In a real implementation, this would disable stack checking */
    /* and set condition codes on return */
}

int main() {
    return 0;
}
