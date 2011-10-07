////////////////////////////////////////////////////////////////////////////////
// taskwarrior - a command line task list manager.
//
// Copyright 2006-2011, Paul Beckingham, Federico Hernandez.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// http://www.opensource.org/licenses/mit-license.php
//
////////////////////////////////////////////////////////////////////////////////


#define L10N                                           // Localization complete.

#include <sstream>
#include <Context.h>
#include <text.h>
#include <util.h>
#include <i18n.h>
#include <main.h>
#include <CmdDuplicate.h>

extern Context context;

////////////////////////////////////////////////////////////////////////////////
CmdDuplicate::CmdDuplicate ()
{
  _keyword     = "duplicate";
  _usage       = "task <filter> duplicate <mods>";
  _description = STRING_CMD_DUPLICATE_USAGE;
  _read_only   = false;
  _displays_id = false;
}

////////////////////////////////////////////////////////////////////////////////
int CmdDuplicate::execute (std::string& output)
{
  int rc = 0;
  int count = 0;
  std::stringstream out;

  // Apply filter.
  std::vector <Task> filtered;
  filter (filtered);
  if (filtered.size () == 0)
  {
    context.footnote (STRING_FEEDBACK_NO_TASKS_SP);
    return 1;
  }

  // Apply the command line modifications to the completed task.
  A3 modifications = context.a3.extract_modifications ();

  std::vector <Task>::iterator task;
  for (task = filtered.begin (); task != filtered.end (); ++task)
  {
    Task dup (*task);
    dup.set ("uuid", uuid ());     // Needs a new UUID.
    dup.setStatus (Task::pending); // Does not inherit status.
    dup.remove ("start");          // Does not inherit start date.
    dup.remove ("end");            // Does not inherit end date.
    dup.remove ("entry");          // Does not inherit entry date.

    // Recurring tasks are duplicated and downgraded to regular tasks.
    if (task->getStatus () == Task::recurring)
    {
      dup.remove ("parent");
      dup.remove ("recur");
      dup.remove ("until");
      dup.remove ("imak");
      dup.remove ("imask");

      out << format (STRING_CMD_DUPLICATE_NON_REC, task->id)
          << "\n";
    }

    modify_task_annotate (dup, modifications);
    context.tdb2.add (dup);
    ++count;

    if (context.verbose ("affected") ||
        context.config.getBoolean ("echo.command")) // Deprecated 2.0
      out << format (STRING_CMD_DUPLICATE_DONE,
                     task->id,
                     task->get ("description"))
          << "\n";

    // TODO This should be a call in to feedback.cpp.
    if (context.verbose ("new-id"))
      out << format (STRING_CMD_ADD_FEEDBACK, context.tdb2.next_id ()) + "\n";

    context.footnote (onProjectChange (dup));
  }

  // TODO Add count summary, like the 'done' command.

  context.tdb2.commit ();
  output = out.str ();
  return rc;
}

////////////////////////////////////////////////////////////////////////////////
