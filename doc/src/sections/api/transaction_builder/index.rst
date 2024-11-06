Transaction Builder
==========================

The Transaction Builder is a utility designed to simplify the process of creating Cardano blockchain transactions. It provides a high-level interface to construct complex transactions by adding inputs, outputs, scripts, metadata, and witnesses. The builder handles the intricate steps of assembling a valid transaction, such as fee calculations, balancing inputs and outputs, and setting necessary scripts. This abstraction allows developers to focus on defining the transaction's components without needing to manage low-level details, making it easier to create robust and secure transactions that adhere to the protocol's constraints.

.. toctree::
    :maxdepth: 1

    ./coin_selector
    ./transaction_balancing
    ./transaction_builder
    ./large_first_coin_selector
    ./coin_selector_impl
    ./fee
    ./implicit_coin
    ./tx_evaluator
    ./tx_evaluator_impl
    ./provider_tx_evaluator