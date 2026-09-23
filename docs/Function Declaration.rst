Function Declaration And Using Response Sequence
================================================

Syntax
------

Declaring Function Syntax:

.. code-block:: baregear

    [functionName] [parameter]:
        [body]

Calling Function Syntax:

.. code-block:: baregear

    [functionName] [parameter]

Example
-------

Declaring Simple Function
--------------------------

A simple function without parameters can be declared using a colon to start the body:

.. code-block:: baregear

    SayHello:
        print "Hello World!"

    SayHello

Adding Parameters
-----------------

Functions can accept parameters. Parameters are specified before the colon:

.. code-block:: baregear

    Square x:
        return x * x

    print Square 2       # Output 4

Optional Parameters
-------------------

Parameters can be marked as optional using the `- optional` syntax. Optional parameters allow the function to be called with or without that parameter:

.. code-block:: baregear

    Square x as numb, y as numb - optional:
        if isAvailable y:
            result = x
            for _ in range y:
                result * x
        return x * x

    print Square 2       # Output 4
    print Square 2, 4    # Output 8

Custom Datatypes
----------------

The `as` keyword is used to specify the datatype of a parameter:

.. code-block:: baregear

    say data:
        print ":" + data

    print Say "Hello"    # Print ":Hello"

Response Sequence
-----------------

Response sequences allow chaining function calls and expressions. The semicolon `;` is used to end function calling parameters in a response sequence:

.. code-block:: baregear

    print "Result" + input "Enter X: " + input "Enter Y: "
    print Square 2; + " is Squared Result Of 2*2" # Output "4 is Squared Result Of 2*2"

Notes
-----

- `- optional`: Used after parameter and datatype declaration to set a parameter as optional
- `as`: Used before `- optional` (if it exists) and after the parameter name to specify datatype
- `;`: Used to end function calling parameters in a Response Sequence
- `isAvailable`: Used to check if an optional parameter was provided