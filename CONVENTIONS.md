# Logger Project Conventions

- These conventions are now used after v4.0.2
- Note that these conventions are not special for this project.

## Keywords

- `[]` optional
- `||` OR
- `&&` AND
- `<field>` Field
- `{field}` Field in field
- `<field: template>` Field with template

# Commit Convention

## Versioning

- Semantic Versioning: `Maj.Min.P`
- First one is major, when it took massive or API-breaking changes
- Second one is minor, when there's big but not massive change and not-breaking changes
- Third one is patch, when there's bug fix, improvement etc.

## Message
```
vX.X[.X]: <subject>

[BREAKING]
[- ...]

<body: - ...>

TESTS
- Tested on <platforms: {arch}-{os} format>
- Verified on [every platform || <platforms: {arch}-{os} format>]
- Safe to use compile in C++ [<exception: on {arch}-{os} platform>]
- Versions: <c-version> <cxx-version>
```

## Example
```
v4.0.2: Time getter syscall fixed for OSX

- Added OSX platform to Makefile
- Added Go usage

TESTS
- Tested on x86_64-linux, x86_64-mingw, x86_64-osx, msvc
            aarch64-linux, aarch64-mingw, aarch64-osx
- Verified on every platform
- Safe to use compile in C++
- Versions: C11, C++17
```

# Release Convention

- Binaries coming from artifacts

## Message
```
<subject>

[# BREAKING]
[- ...]

# Tests
- Tested on <platforms: {arch}-{os} format>
- Verified on [every platform || <platforms: {arch}-{os} format>]
- Safe to use compile in C++ [<exception: on {arch}-{os} platform>]
- Versions: <c-version> <cxx-version>

See this commit: https://github.com/ilpenSE/logger/commit/$hash
```

## Example Message
```
Functions and macros with explicit instance are changed

# BREAKING
- All explicit instance macros like this: lg_level_to
- All explicit instance functionslike this: lg_flevel_to

# Tests
- Tested on x86_64-linux, x86_64-mingw, x86_64-osx, msvc
            aarch64-linux, aarch64-mingw, aarch64-osx
- Verified on every platform
- Safe to use compile in C++
- Versions: C11, C++17

See this commit: https://github.com/ilpenSE/logger/commit/HASH_HERE
```
