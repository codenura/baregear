Data Types
----------

baregear allows the dynamic typing
but you can limit to use the datatype
of variable.

Syntax
------

.. code-block:: baregear

    [datatypes], [more datatypes] [variableName]

Example
-------

Simple Calculator Has Bug If User Typed The Text
When User Enter Text Instead Of Number:

.. code-block:: bash

    Enter First Number: hi
    Enter Operator: -
    Operator Not Supported
    Enter Operator: *
    Operator Not Supported
    Enter Operator: /
    Operator Not Supported
    Enter Operator: +
    Enter Second Number: 48
    Result hi48

You Can Use numb to block the inputting text:

.. code-block:: baregear

    numb x = input "Enter Your First Number: "
    op = input "Enter Your Operator To Calculate: "
    numb y = input "Enter Your Second Number: "

    # Next Code Remains Same

After Applying The Code:

.. code-block:: bash

    Enter First Number: h
    Only Numbers Allowed.
    Enter First Number:

List Of Datatypes:

- variant datatype variable supports all datatypes
- `text`: Allow To Store And Load String
- `numb`: Supports `int`, `float`, `double`, `short`, `long` Value Store
  And Loading And Auto Dynamic Allocation
- `short`: Used to store the the value in limit but not support dynamic Allocation
- `double`: Used to store accurate decimal Numbers
- `float`: Used to store the decimal Numbers
- `int`: Used To Store Only Non decimal number
- `bool`: Used To Store True/False Values

And Guess The Number Game Make To Easier To Gess By User:

.. code-block:: baregear

    attempts = 0
    int number = random 1 to 100
    print "Guess The Number From 1 to 100 But You Can Provide Number Upto 5 Times"

    while attempts < 5:
        int userInput = input "Enter Your Number: ".
        if userInput > number:
            print "Your Number Greater Then My Number."
        else if userInput < number:
            print "Your Number Shorter Then My Number."
        else:
            print "Matched."
            exit 0
        attempts += 1

    print "You Cannot Guess Valid Number."

Creating Custom Variant Variable:

.. code-block:: baregear

    text, int alphabetNumber

And When You Store Number With Decimal Or Text on 'alphabetNumber'
To Assign as text Otherwise You Store Only Number To Assign as int

Using Boolean Variable:

.. code-block:: baregear
    
    bool x = true
    if x:
        print "X is true"

    bool y = false
    if y:
        print "Y is true"