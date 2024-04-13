Address
==========================

Addresses in Cardano are sequences of bytes that conform to a specific format, designed to efficiently encapsulate information about ownership and stake delegation.

This section of the documentation delves into the structure and usage of addresses within the Cardano ecosystem. It covers addresses from both the Shelley and Byron eras, highlighting their distinct characteristics and encoding methods. Each subsection below provides detailed information on how to generate, parse, and interact with different types of Cardano addresses provided by this library:

.. toctree::
    :maxdepth: 1

    ./address
    ./address_type
    ./base_address
    ./byron_address
    ./byron_address_attributes
    ./byron_address_type
    ./enterprise_address
    ./pointer_address
    ./stake_pointer
    ./reward_address

Addresses are typically encountered by users in encoded form. Shelley era addresses utilize Bech32 encoding, which allows for seamless operation without a strict length limitation, and features a built-in error detection system to prevent common input errors. On the other hand, Byron addresses retain the Base58 encoding to distinguish them easily from newer address formats and provide backward compatibility.

Each address type supports different features and uses within the Cardano network, from receiving funds to controlling stake rewards. The following sections detail each address type's specifications and functionalities, ensuring developers and users can fully leverage the capabilities of Cardano's addressing system.