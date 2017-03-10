The interfaces in this folder define a thin wrapper around D3D11 and D3D12 APIs that
are used internally by the Flex solver. Some parts of the Flex API (e.g.: solver callbacks)
will return pointers to these structures.

See the extensions forcefield API for an example of how these wrappers may be used to implement
Flex solver callbacks. Note that this code should not be modified or recompiled to ensure binary
compatibility with the main Flex libraries.
