#include "JournalWrapper.h"
#include <stdexcept>
#include <cstring>

/**
 * @brief Read a specific field from the current journal entry.
 *
 * systemd journal stores each field as "KEY=VALUE" in a raw memory buffer.
 * This helper extracts only the VALUE part and converts it to std::string.
 *
 * @param j      Pointer to an opened sd_journal instance, already positioned
 *               at a valid entry (after sd_journal_next()).
 * @param field  Name of the journal field to read (e.g. "MESSAGE", "PRIORITY").
 *
 * @return
 *   - std::string containing the field value (without "KEY=" prefix)
 *   - empty string if:
 *       + field does not exist in this entry
 *       + data format is unexpected
 *
 * Design notes:
 * - sd_journal_get_data() returns a pointer owned by systemd, not by caller.
 * - We must NOT free `data`.
 * - Journal fields are not guaranteed to exist for every entry.
 */
std::string JournalReader::ReadEntryField(sd_journal* j, const char* field)
{
    const void* data;
    size_t len;

    if (sd_journal_get_data(j, field, &data, &len) < 0)
    {
        return {};
    }

    const char* s = static_cast<const char*>(data);
    const char* eq = strchr(s, '=');
    if (!eq)
    {
        return {};
    }

    return std::string(eq + 1, len - (eq - s) - 1);
}

/**
 * @brief Construct a JournalReader and open a systemd journal source.
 *
 * @param mode  Defines which journal namespace is opened:
 *              - SYSTEM    : system-wide journal
 *              - USER      : current user's journal
 *              - DIRECTORY : journal files located in a specific directory
 *
 * @param path  Used only when mode == DIRECTORY.
 *
 * Behavior:
 * - Opens the journal immediately.
 * - Throws std::runtime_error on failure.
 *
 * Design rationale:
 * - JournalReader is RAII-based: open in constructor, close in destructor.
 * - If journal cannot be opened, the object is considered invalid.
 */
JournalReader::JournalReader(OpenMode mode, const std::string& path)
{
    int lresult = -1;

    switch (mode)
    {
        case OpenMode::SYSTEM:
            lresult = sd_journal_open(&m_journal, SD_JOURNAL_SYSTEM);
            break;

        case OpenMode::USER:
            lresult = sd_journal_open(&m_journal, SD_JOURNAL_CURRENT_USER);
            break;

        case OpenMode::DIRECTORY:
            lresult = sd_journal_open_directory(&m_journal, path.c_str(), 0);
            break;
    }

    if (lresult < 0 || !m_journal)
    {
        throw std::runtime_error("Failed to open systemd journal");
    }
        
}

/**
 * @brief Close the journal and release systemd resources.
 *
 * Notes:
 * - sd_journal_close() is safe to call only once.
 * - Destructor guarantees no journal FD leak.
 */
JournalReader::~JournalReader()
{
    if (m_journal)
    {
        sd_journal_close(m_journal);
    }
}

/**
 * @brief Add a match filter to the journal.
 *
 * @param match Filter expression in systemd format,
 *              e.g. "_SYSTEMD_UNIT=ssh.service"
 *
 * Effect:
 * - Only entries matching ALL added filters will be returned by sd_journal_next().
 *
 * Important:
 * - Filters are cumulative.
 * - This does NOT reposition the read pointer.
 */
void JournalReader::AddMatch(const std::string& match)
{
    sd_journal_add_match(m_journal, match.c_str(), 0);
}

const char* JournalReader::toFieldName(JournalField f)
{
    switch (f)
    {
        case JournalField::SystemdUnit:      return "_SYSTEMD_UNIT";
        case JournalField::Pid:              return "_PID";
        case JournalField::SyslogIdentifier: return "SYSLOG_IDENTIFIER";
        case JournalField::Priority:         return "PRIORITY";
        case JournalField::Message:          return "MESSAGE";
    }
    return "";
}

void JournalReader::AddMatch(const JournalField& f, const std::string& value)
{
    std::string lfield = std::string(toFieldName(f));
    if(lfield.empty() == false)
    {
        std::string expr = lfield + "=" + value;
        sd_journal_add_match(m_journal, expr.c_str(), 0);
    }
    else
    {
        sd_journal_add_match(m_journal, value.c_str(), 0);
    }
}

/**
 * @brief Remove all previously added match filters.
 *
 * Effect:
 * - Journal returns to unfiltered mode.
 */
void JournalReader::ClearMatches()
{
    sd_journal_flush_matches(m_journal);
}

/**
 * @brief Seek to the beginning of the journal.
 *
 * After calling this, the next call to next() will return
 * the oldest available journal entry.
 */
void JournalReader::SeekHead()
{
    sd_journal_seek_head(m_journal);
}

/**
 * @brief Seek to the end of the journal.
 *
 * Typical usage:
 * - Call seekTail()
 * - Then call next() to read only NEW entries (tail -f behavior)
 */
void JournalReader::SeekTail()
{
    sd_journal_seek_tail(m_journal);
}

/**
 * @brief Seek to a specific realtime timestamp.
 *
 * @param usec Timestamp in microseconds since UNIX epoch.
 *
 * Effect:
 * - The next next() will return the first entry at or after this time.
 */
void JournalReader::SeekRealtime(uint64_t usec)
{
    sd_journal_seek_realtime_usec(m_journal, usec);
}

/**
 * @brief Seek to a specific journal cursor.
 *
 * @param cursor Cursor string previously obtained from sd_journal_get_cursor().
 *
 * @return true  if seek succeeded
 * @return false if cursor is invalid or not found
 *
 * Use case:
 * - Resume reading after restart without re-reading old entries.
 */
bool JournalReader::SeekCursor(const std::string& cursor)
{
    return sd_journal_seek_cursor(m_journal, cursor.c_str()) >= 0;
}

/**
 * @brief Advance to the next journal entry and extract its data.
 *
 * @param entry Output structure to receive parsed journal fields.
 *
 * @return
 *   - true  : a new entry was read successfully
 *   - false : no more entries or error
 *
 * Behavior:
 * - Moves the internal journal cursor forward by one entry.
 * - Extracts commonly used fields into JournalEntry.
 * - Captures the entry cursor for resume capability.
 *
 * IMPORTANT:
 * - sd_journal_next() must be called before any field access.
 * - This function NEVER returns the same entry twice.
 */
bool JournalReader::NextEntry(JournalEntry& entry)
{
    int r = sd_journal_next(m_journal);
    if (r <= 0)
        return false;

    entry.message     = ReadEntryField(m_journal, toFieldName(JournalField::Message));
    entry.unit        = ReadEntryField(m_journal, toFieldName(JournalField::SystemdUnit));
    entry.identifier  = ReadEntryField(m_journal, toFieldName(JournalField::SyslogIdentifier));
    entry.priority    = ReadEntryField(m_journal, toFieldName(JournalField::Priority));

    char* cursor = nullptr;
    if (sd_journal_get_cursor(m_journal, &cursor) >= 0 && cursor)
    {
        entry.cursor = cursor;
        free(cursor);
    }
    else
    {
        entry.cursor.clear();
    }

    sd_journal_get_realtime_usec(m_journal, &entry.realtime_usec);

    return true;
}

/**
 * @brief Wait for new journal entries.
 *
 * @param timeout_ms Timeout in milliseconds.
 *
 * @return true  if new data is available
 * @return false if timeout expired or error occurred
 *
 * Typical usage:
 * - Used together with seekTail() to implement follow-mode.
 */
bool JournalReader::Wait(int timeout_ms)
{
    return sd_journal_wait(m_journal, timeout_ms * 1000) > 0;
}

/*
sd_journal_get_catalog
sd_journal_get_catalog_for_message_id
*/