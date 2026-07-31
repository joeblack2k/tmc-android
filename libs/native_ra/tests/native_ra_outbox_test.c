#include "../src/native_ra_outbox.h"

#include <stdio.h>

int main(void) {
    const char* url = "https://retroachievements.org/dorequest.php";
    const char* type = "application/x-www-form-urlencoded";
    if (nra_outbox_classify_request(url, "r=awardachievement&u=user&t=token&a=5&h=0&v=0123456789abcdef0123456789abcdef", type) != NRA_OUTBOX_ACHIEVEMENT ||
        nra_outbox_classify_request(url, "r=submitlbentry&u=user&t=token&i=4&s=-3&v=0123456789abcdef0123456789abcdef", type) != NRA_OUTBOX_LEADERBOARD ||
        nra_outbox_classify_request(url, "r=login2&u=user&t=token", type) != NRA_OUTBOX_NONE ||
        nra_outbox_classify_request("https://example.invalid/dorequest.php", "r=awardachievement&u=user&t=token&a=5&h=0&v=0123456789abcdef0123456789abcdef", type) != NRA_OUTBOX_NONE) {
        return 1;
    }
    puts("NATIVE RA OUTBOX CLASSIFIER OK");
    return 0;
}
