#include "main.h"
#include "JournalWrapper.h"

int main(void)
{
    JournalReader reader(JournalReader::OpenMode::SYSTEM);

    // reader.AddMatch("_SYSTEMD_UNIT=myservice.service");
    reader.SeekHead();

    while (true)
    {
        JournalEntry e;

        // 1. Đọc hết log đang có
        while (reader.NextEntry(e))
        {
            printf("[%lu] %s\n", e.realtime_usec, e.message.c_str());
        }

        // 2. Chờ log mới
        reader.Wait(1000); // 1s
    }
}