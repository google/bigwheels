# C++ Style Guide

<!-- @import "[TOC]" {cmd="toc" depthFrom=2 depthTo=6 orderedList=false} -->

<!-- code_chunk_output -->

- [Variable initialization](#variable-initialization)
- [Variable declaration](#variable-declaration)
- [Use clang-format whenever possible](#use-clang-format-whenever-possible)
  - [clang-format off/on](#clang-format-offon)
- [Spacing and alignment](#spacing-and-alignment)
  - [Function parameter and argument spacing](#function-parameter-and-argument-spacing)
  - [Flow control statements](#flow-control-statements)
- [Naming Conventions](#naming-conventions)
  - [Macros](#macros)
  - [Enums](#enums)
  - [Constants](#constants)
  - [Type Names](#type-names)
    - [Exceptions to type names](#exceptions-to-type-names)
  - [Parameter and variable names](#parameter-and-variable-names)
    - [Exceptions to parameter and variable names](#exceptions-to-parameter-and-variable-names)
  - [Global and static variables](#global-and-static-variables)
  - [Pointer variable names](#pointer-variable-names)
    - [Exceptions to pointer variable names](#exceptions-to-pointer-variable-names)
  - [Class member variable names](#class-member-variable-names)
  - [Struct member variable names](#struct-member-variable-names)
  - [Functions](#functions)
- [Parameters and arguments](#parameters-and-arguments)
  - [Single line function declarations and calls](#single-line-function-declarations-and-calls)
  - [Multiple line function declarations and calls](#multiple-line-function-declarations-and-calls)
  - [Do not use bin/paragraph argument packing](#do-not-use-binparagraph-argument-packing)
    - [Exception to not using bin/paragraph argument packing](#exception-to-not-using-binparagraph-argument-packing)
- [Braces](#braces)
  - [Enums](#enums-1)
  - [Functions](#functions-1)
  - [Classes and structs](#classes-and-structs)
  - [Flow control statements](#flow-control-statements-1)
    - [Do not use Java style if/else](#do-not-use-java-style-ifelse)
- [Comments](#comments)
- [Usage of C++ features](#usage-of-c-features)

<!-- /code_chunk_output -->

## Variable initialization
All variables must be explicitly initialized at declaration unless there is a particular reason not to do so. This includes class and struct member variables. Objects that have a default constructor that initializes the object to a well defined state do not need explicit initialization.

```c++
// Local variables
int     x       = 0;
Device* pDevice = nullptr;

// Class members
class MyClass {
private:
    int              mMode   = 0;
    Device*          mDevice = nullptr;
    std::vector<int> mIndicators; // No need to initizlie this
};

// Struct members
struct MyStruct {
    int     mode    = 0;
    Device* pDevice = nuullptr;
};

// Fill in struct used with functions - no need to initialize these
struct FillInStruct {
    int         deviceId;
    const char* deviceName;
};
```

## Variable declaration

No more than one variable per line.

```c++
// Good
int   x     = 0;
int   y     = 720;
float depth = 1.0f;
float scale = 2.0f;

// BAD
int   x, y;
float depth, scale;

// BAD
int   x = 0, y = 720;
float depth = 1.0f, scale = 2.0f;
```

## Use clang-format whenever possible

BigWheels has a `.clang-format` file in the root of the project. It's recommended that all code contributions use it. The resulting formatted code should cover the majority of spacing, alignment and argument handling styles mentioned in this doc.

### clang-format off/on

`clang-format off` and `clang-format on` should be use wherever makes sense. For instance specifying geometry data.

Avoid using  `clang-format off` and `clang-format on` to circumvent coding guidelines. 

If someone has put `clang-format off` and `clang-format on` tags around a block of code, please do not remove it without consulting them. There is a reason that they placed it there in the first place and that should be respected.

## Spacing and alignment

Use 4 space indentions. No tabs.

Variable declaration and assignment should be aligned to the block they're declared in.

```c++
// Block one
int     x       = 0;
int     y       = 720;
float   depth   = 1.0f;
float   scale   = 2.0f;
int     mode    = 0;
Device* pDevice = nullptr;

// Block two
double        time         = 0;
int           mode         = 1;
ResourceState initialState = SOME_RESOURCE_STATE;

// Block three
VkDevice deviceHandle = VK_NULL_HANDLE;
VkResult vkres        = vkCreateDevice(..., &deviceHandle);
```

If the type name or variable causes the initial assignment to be visually jarring, a `//` can be used in between to maintain reasier readability.

```c++
// This might look jarring
VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeatures = {}; 
auto                                       res                        = GetFeatures(descriptorIndexingFeatures);

// Use empty comment break assignment alignment for easier readabiilty
VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeatures = {}; 
//
auto res = GetFeatures(descriptorIndexingFeatures);
```

### Function parameter and argument spacing
Function declaration and calls should only have spacing between a comma and the next parameter or argument. There should not be a space between the opening paren and the first argument nor should be there be a space between the closing paren and the last argument.

```c++
// Declaration
void MyFunction(int a, int b, int c);

// Call
MyFunction(0, 1, 2);
```

### Flow control statements
There must be a single space between the keyword and the open parent of the a flow control statement.

```c++
// for loops
for (int x = 0; x < 10; ++x) {
}

// while loops
while (cond) {
}

// do-while loops
do {
} while (cond);

// if/else
if (cond) {
} 
else {
}

// if/else if/else
if (cond1) {
}
else if (cond2) {    
}
else {
}

// switch
switch (x) {
}
```

## Naming Conventions

### Macros

Macros and macro arguments should be all upper case and use underscore to separate words.

```c++
#define MACRO(ARG0, ARG1, ARG2)

#define ANOTHER_MACRO(OBJ_NAME, SOME_OTHER_ARG)
```

### Enums

Enum type names should be camel cased with the first letter upper cased. Enum values should be all upper cased and use underscore to separate words. Enum names must have the enun type prefixed.

Enum should specify values to aid debugging, but there are cases where this is impractical and values can be omitted. Use your best judgment.

```c++
enum HeapType {
    HEAP_TYPE_DEFAULT = 0,
    HEAP_TYPE_CUSTOM  = 1,
};
```

### Constants

Constant names shoudl use one of the following two conventions:
 1. Constant names are upper cased and use underscore to separate words. 
 1. Constant names are camel cased with a `k` prefix.

```c++
const int WINDOW_SIZE = 100;
const int kWindowSize = 100;
```

### Type Names

Type names should be camel cased with the first letter upper cased.

```c++
// Aliased type names
using ObjectPtr = std::unique_ptr<Object>;
typedef std::unique_ptr<Object> ObjectPtr;

// Class name
class Device;
class CommandBuffer;
class Swapchain;

// Struct name
struct DeviceCreateInfo;
struct PresentInfo;
```

#### Exceptions to type names

Math types that reflect their HLSL or GLSL counterparts should keep the same type names.

```c++
// HLSL like type names
float2   uv;
float3   normal;
float4   positionWS; 
float3x3 normalMatrix;
float4x4 mvpMatrix;

// GLSL like type names
vec2 uv;
vec3 normal;
vec4 positionWS;
mat3 normalMatrix;
mat4 mvpMatrix;
```

### Parameter and variable names

Function parameter and local variable names should be camel cased with with the first letter lower cased.

```c++
void MyFunction(int someValue, int someOtherValue)
{
    int nextSomeValue      = someValue + 1;
    int nextSomeOtherValue = someOhterValue + 1;
}
```

#### Exceptions to parameter and variable names

If the parameter or variable name looks like it might cause some ambiguity, pick a reasonable casing alternative to improve readability.

```c++
// Possibly ambiguous names - avoid parameter and variable names like these
void MyFunction(float4x4 mVPMatrix); // mVPMatrix looks too much like class member variable name
float3 positionWs;                   // poisitionWs looks too much like the pural of position W


// Reasonable alternative
void MyFunction(float4x4 mvpMatrix); // More clearly MVP matrix
float3 postionWS;                    // More clearly position WS (World Space)
```

### Global and static variables

Global variables can have a `g` prefix but it is not required. Static variables can have an `s` prefix but it is not required.

```c++
int gDelta = 0;

static bool sInitialized = false;
```

### Pointer variable names

Pointer variable names should be camel cased with a `p` prefix.

```c++
// Pointer variables
int*    pValue  = nullptr;
Device* pDevice = pMyDevice;

// Pointer argument
void MyFunction(CommandBuffer* pCmd);

// Pointers in structs
struct MyStruct
{
    Window* pWindow = nullptr;
};

```

#### Exceptions to pointer variable names

There are some exceptions to the pointer variable naming convention:
 * `const char*` strings
 * Class member variable names - see the section below on class member variable names.

`const char*` strings do not need the `p` prefix.

```c++
const char* extensionName = "My Extension Name";
```


### Class member variable names

Class member variable names should be camel cased with an `m` prefix.

```c++
class MyClass
{
private:
    int     mMode   = 0;
    Window* mWindow = nullptr;
};
```

### Struct member variable names

Struct member variable names should be camel cased with the first letter lower cased.

```c++
struct MyStruct
{
    int     mode = 0;
    Window* pWindow = nullptr;
};
```

### Functions

Functions names should be camel cased with the first letter upper cased.

```c++
void MyStandAloneFunction();
```

```c++
class MyClass
{
public:
    MyClass();
    ~MyClass();

    void MyMemberFunction();
}
```

## Parameters and arguments

### Single line function declarations and calls

Function declarations and calls can be on a single line if the entire function signature or call can reasonably fit online while maintaining readablity. 

```c++
// Function declaraiotn
void MyFunction(int param0, const char* param1, float param2);

// Function call
int         modeSelect  = 0;
const char* targetName  = "MyTarget";
float       scaleFactor = 0.01f;

MyFunction(modeSelect, param1, scaleFactor);
```

### Multiple line function declarations and calls

Function declarations and calls in which the entirety of the function signature or call cannot fit on one line should each place parameters and arguments in individual lines. Parameter type and names should be aligned except in the case of function pointers.

```c++
// Function declaration
void MyFunction(
    CommandBuffer* pCommandBuffer,
    Image*         pImage,
    uint32_t       firstMipLevel,
    uint32_t       numMipLevels,
    uint32_t       firstArrayLayer,
    uint32_t       numArrayLayers,
    ResourceState  stateBefore,
    ResourceState  stateAfter);

// Function call
MyFunction(
    pDrawCmd,
    pBountyTex,
    0,
    1,
    0,
    1,
    initalState,
    finalState);
```

Decorations can be used to help the reader know which parameter the argument belongs to.

```c++
// Decoration style 1
MyFunction(
    pDrawCmd,    // pCommandBuffer
    pBountyTex,  // pImage
    0,           // firstMipLevel
    1,           // numMipLevels
    0,           // firstArrayLayer
    1,           // numArrayLayers
    initalState, // stateBefore
    finalState); // stateAfter

// Decoration style 2 - keep ending */ and arg names aligned
MyFunction(
    /* pCommandBuffer  */ pDrawCmd,
    /* pImage          */ pBountyTex,
    /* firstMipLevel   */ 0,
    /* numMipLevels    */ 1,
    /* firstArrayLayer */ 0,
    /* numArrayLayers  */ 1,
    /* stateBefore     */ initalState,
    /* stateAfter      */ finalState);
```

### Do not use bin/paragraph argument packing

Avoid using bin/paragraph argument packing as it can make argument substituion difficult while debugging.

```c++
// Do not pack arguments this way
void MyFunction(arg0, arg1, 
    arg2, arg3);

// Do not pack arguments this way
void MyFunction(
    arg0, arg1, 
    arg2, arg3);
    
// Do not pack arguments this way
void MyFunction(
    arg0, arg1, arg2, arg3, 
    arg4, arg5, arg7, arg7);    
```

#### Exception to not using bin/paragraph argument packing

Bin/paragraph argument packing can be used when specifying values for matrices. Columns and rows should be aligned for readability.

```c++
// Try not to do this
float2x2 MyMat(e0, e1,
    e2, e3);

// 2x2 matrix
float2x2 MyMat(
    e0, e1,
    e2, e3);

// 2x3 matrix
float2x2 MyMat(
    e0, e1, e2,
    e3, e4, e5);

// 3x3 matrix
float3x3 MyMat(
    e0, e1, e2,
    e3, e4, e5,
    e5, e7, e8);
```

## Braces

### Enums
Braces for enum blocks should be on the line after the full declaration.
```c++
enum MyFlags
{
    MY_FLAG_NONE   = 0,
    MY_FLAG_FIRST  = 1,
    MY_FLAG_SECOND = 2,
};
```

### Functions
Braces for function bodies should be on the line after the function signature.
```c++
void MyFunction()
{
    int x = 1;    
}
```
### Classes and structs
Braces for classes and structs should be on the line after the full declaration.
```c++
class MyClass
{
};

struct MyStruct
{
};
```

### Flow control statements
Braces for flow control statements should come at the end of the line. 
*NOTE: This may change in the future to move the brace to the line after the flow control statement.*
```c++
// for loops
for (int x = 0; x < 10; ++x) {
}

// while loops
while (cond) {
}

// do-while loops
do {
} while (cond);

// if/else
if (cond) {
} 
else {
}

// if/else if/else
if (cond1) {
}
else if (cond2) {    
}
else {
}

// switch
switch (x) {
}
```
#### Do not use Java style if/else
```c++
// Avoid this
if (cond) {
} else {
}

// Avoid this
if (cond1) {
} else if (cond2) {    
} else {
}
```

## Comments
Both C and C++ style comments can be used freely.  

*NOTE: While there are some Doxygen style comments for classes, these may change in the future to remove the Doxygen tags Doxygen isn't to generate documentation for BigWheels.*

## Usage of C++ features

BigWheels permits the use of C++ features up to the version of C++ that is specified in the top level `CMakeList.txt` file. The only C++ feature that that must not be used directly is exceptions. While the STL and possibly some of the dependencies might make use of exceptions, BigWheels does not.

Please take caution when using an esoteric C++ feature. This can create difficult debugging scenarios later.

Please take caution with extensive use the STL. Common data structures like `std::vector` can be used freely. However, algorithms that make heavy use of C++ templates can be very difficult to debug. If the use of theses algorithms are unavoidable - incude a comment expressing the intent of the code.