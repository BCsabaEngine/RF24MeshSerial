/**
   SerialCommand - A Wiring/Arduino library to tokenize and parse commands
   received over a serial port.

   Copyright (C) 2021 Csaba Balazs
   Copyright (C) 2012 Stefan Rado
   Copyright (C) 2011 Steven Cogswell <steven.cogswell@gmail.com>
                      http://husks.wordpress.com

   Version 20210320

   This library is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "SerialCommand.h"

/**
   Constructor makes sure some things are set.
*/
SerialCommand::SerialCommand()
  : commandCount(0),
    defaultHandler(NULL),
    nullHandler(NULL),
    term('\n'),
    term2('\r'),
    last(NULL)
{
  strcpy(delim, " "); // strtok_r needs a null-terminated string
  clearBuffer();
}

/**
   Adds a "command" and a handler function to the list of available commands.
   This is used for matching a found token in the buffer, and gives the pointer
   to the handler function to deal with it.
*/
void SerialCommand::addCommand(const char *command, void (*function)()) {
#ifdef SERIALCOMMAND_DEBUG
  Serial.print("Adding command (");
  Serial.print(commandCount);
  Serial.print("): ");
  Serial.println(command);
#endif

  if (commandCount >= SERIALCOMMAND_MAXCOMMANDCOUNT)
    return;

  strncpy(commandList[commandCount].command, command, SERIALCOMMAND_MAXCOMMANDLENGTH);
  commandList[commandCount].function = function;
  commandCount++;
}

/**
   This sets up a handler to be called in the event that the receveived command string
   isn't in the list of commands.
*/
void SerialCommand::setDefaultHandler(void (*function)(const char *)) {
  defaultHandler = function;
}

/**
   This sets up a handler to be called in the event that the user hits enter on a
   blank line
*/
void SerialCommand::setNullHandler(void (*function)()) {
  nullHandler = function;
}

/**
   This checks the Serial stream for characters, and assembles them into a buffer.
   When the terminator character (default '\n') is seen, it starts parsing the
   buffer for a prefix command, and calls handlers setup by addCommand() member
*/
void SerialCommand::readSerial() {
  while (Serial.available() > 0) {
    char inChar = Serial.read();   // Read single available character, there may be more waiting
#if defined SERIALCOMMAND_DEBUG || defined SERIAL_COMMAND_ECHO
    Serial.print(inChar);   // Echo back to serial stream
#endif

    if ((inChar == term) || (inChar == term2)) {     // Check for the terminator (default '\r') meaning end of command
#ifdef SERIALCOMMAND_DEBUG
      Serial.print("Received: ");
      Serial.println(buffer);
#endif

      char *command = strtok_r(buffer, delim, &last);   // Search for command at start of buffer
      if (command != NULL) {
        bool matched = false;
        for (int i = 0; i < commandCount; i++) {
#ifdef SERIALCOMMAND_DEBUG
          Serial.print("Comparing [");
          Serial.print(command);
          Serial.print("] to [");
          Serial.print(commandList[i].command);
          Serial.println("]");
#endif

          // Compare the found command against the list of known commands for a match
          if (strncmp(command, commandList[i].command, SERIALCOMMAND_MAXCOMMANDLENGTH) == 0) {
#ifdef SERIALCOMMAND_DEBUG
            Serial.print("Matched Command: ");
            Serial.println(command);
#endif

            // Execute the stored handler function for the command
            (*commandList[i].function)();
            matched = true;
            break;
          }
        }
        if (!matched && (defaultHandler != NULL)) {
          (*defaultHandler)(command);
        }
      }
      else {
        if (nullHandler)
          (*nullHandler)();
      }
      clearBuffer();
    }
    else if (isprint(inChar)) {     // Only printable characters into the buffer
      if (bufPos < SERIALCOMMAND_BUFFER) {
        buffer[bufPos++] = inChar;  // Put character into buffer
        buffer[bufPos] = '\0';      // Null terminate
      } else {
#ifdef SERIALCOMMAND_DEBUG
        Serial.println("Line buffer is full - increase SERIALCOMMAND_BUFFER");
#endif
      }
    }
    else if (inChar == 127 || inChar == 8) {
      if (bufPos)
      {
        bufPos--;
        buffer[bufPos] = '\0';
      }
    }
  }
}

/*
   Clear the input buffer.
*/
void SerialCommand::clearBuffer() {
  buffer[0] = '\0';
  bufPos = 0;
}

/**
   Retrieve the next token ("word" or "argument") from the command buffer.
   Returns NULL if no more tokens exist.
*/
char *SerialCommand::next() {
  return strtok_r(NULL, delim, &last);
}
