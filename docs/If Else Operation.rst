If/Else Operation
=================

Syntax
------
If Else Operation Syntax:

.. code-block:: baregear

    [if/else/else if/elif] <condition>:
        [body]

Example
-------

If/Else Operation Is Used For Implement
The Logic Of Code To Work Properly For
Example: You Have 0.081 Dollar If
Biscuit Price Shorter Or Equal To
You Buy, Otherwise You Return To Your Home,
Let's Understand By Creating Project

.. code-block:: baregear

    money = 0.081
    if money >= input "Enter The Biscuit Value":
        print "I Bought Biscuit"
    else
        print "I Come Back To Home"

Result If Equals Or Shorter:

.. code-block:: baregear

    Enter The Biscuit Value: 0.081
    I Bought Biscuit

Result Is Not Equals:

.. code-block:: baregear

    Enter The Biscuit Value: 0.16
    I Come Back To Home

Lets Create Project

1. Simple Calculator:
~~~~~~~~~~~~~~~~~~~~~

.. code-block:: baregear

    x = input "Enter Your First Number: "
    op = input "Enter Your Operator To Calculate: "
    y = input "Enter Your Second Number: "

    switch op:
        case '+':
            print 'Result: ' + (x + y)

        case '-':
            print 'Result: ' + (x - y)

        case '*':
            print 'Result: ' + (x * y)

        case '/':
            print 'Result: ' + (x / y)

        default:
            print 'Invalid Operator: ' + op

But You Won't Implement The if, else for fixing the
problem of Divide By Zero Or Any Compiler Handle It,
And Compiler Also Implement User If Typed Wrong Operator
To Doesn't Accept And Show Error ``'Invalid Operator Please
Enter Operator From +, -, *, /'``
When User Typed Any Operator To Automatically Take
Without Pressing Enter Key And If Bugs Found By Simulating
The Program To Fix By Compiler If Solution Available
Otherwise Throw Error.

Then Use Same Compilation Step That Get From Quick Start Documentation

Output Looks Like:

.. code-block:: bash

    Enter First Number: 49
    Enter Operator: *
    Enter Second Number: 273
    Result: 13377

If User Do Mistake:

.. code-block:: bash

    Enter First Number: 49
    Enter Operator: -:-
    Invalid Operator Please Enter Operator From +, -, *, /
    Enter Operator: /
    Enter Second Number: 0
    Cannot Divide By Zero
    Enter Second Number: 2
    Result: 24.5

2. Guess The Number Game
~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: baregear

    attempts = 0
    number = random 1 to 100
    print "Guess The Number From 1 to 100 But You Can Provide Number Upto 5 Times"

    while attempts < 5:
        userInput = input "Enter Your Number: ".
        if userInput > number:
            print "Your Number Greater Then My Number."
        else if userInput < number:
            print "Your Number Shorter Then My Number."
        else:
            print "Matched."
            exit 0
        attempts += 1

    print "You Cannot Guess Valid Number."

- Alternative You Can Use switch
- Alternative else if keyword is 'elif'