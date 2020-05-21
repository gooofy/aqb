REM constant visibility test

OPTION EXPLICIT

CONST AS INTEGER c=42

SUB foo
    ASSERT c=42
END SUB

ASSERT c=42

foo

