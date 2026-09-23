Contributing Guidelines
=======================

1. Statement Usage Rule
-----------------------

Don't Use Curly Braces In Single Instruction Of Statement Like :

❌ With Curly Braces On Single Instruction On Statement:

.. code-block:: cpp

    if (true) {
        return;
    }

✅ Without Curly Braces On Single Instruction On Statement:

.. code-block:: cpp

    if (true)
        return;

2. Commenting
-------------

Commenting Is Good For Make To Easy To Understand The Logic, But
Don't Use To Explain The Code If Logic Is Easy To Understandable By Others.
For Example:

.. code-block:: cpp

    dynvar val;          // Decleare Dynamic Value
    setValue(&val, (lgr)"Hello World"); // Store Value On val Variable

And Don't Comment In Non Raw Format Because It Can be Hard To Understand

3. AI Agent
-----------

Specify The AI Agent Name And LLM Name And Place On
Vibecoded Line. For Example:

.. code-block:: cpp

    /*
     * Agent: Corsor
     * LLM: Claude 4 Sonnet
     */

Multiple Agents And LLM Example

.. code-block:: cpp

    /*
     * Agent: Corsor, GitHub Copilot, Devin, KiloCode
     * LLM: Claude 4 Sonnet, Gemini 3.1 Pro, GPT-5.6 Luna, Claude Fable 5.1, Kimi K3
     */

Note: AI Model Is Not Required To Specify If You Send
Prompt Like 'Remove Reference Of x Function' To AI Model

4. Optimization And Performance
-------------------------------

Don't Use Heap Based Allocated Datatypes If
Not Required To Call Because It Can Take The
More Build Time And Make Custom Runtime To
Increase Compilation Performance The Function Like:

.. code-block:: cpp

    // ❌ Slow Allocation In Normal C/C++ Runtime
    std::string val = "Hello World";

    // ✅ Fast Allocation
    dynvar val;
    setValue(&val, (lgr)"Hello World");