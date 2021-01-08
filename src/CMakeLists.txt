set(QwertycoinTools_INCLUDE_DIRS
    ${QwertycoinFramework_INCLUDE_DIRS}
    "${CMAKE_CURRENT_LIST_DIR}"
)

set(QwertycoinTools_INCLUDE_DIRS ${QwertycoinTools_INCLUDE_DIRS} PARENT_SCOPE)

# QwertycoinTools

add_custom_target(QwertycoinTools)
add_dependencies(QwertycoinTools
    QwertycoinTools::Daemon
    QwertycoinTools::PaymentGateService
    QwertycoinTools::SimpleWallet
)

# QwertycoinTools::BinaryInfo # NOTE: Ignore this. It's not a target.

# QwertycoinTools::Daemon

set(QwertycoinTools_Daemon_SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/Daemon/Daemon.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Daemon/DaemonCommandsHandler.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Daemon/DaemonCommandsHandler.h"
)

if(WIN32)
    list(APPEND QwertycoinTools_Daemon_SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/BinaryInfo/daemon.rc"
    )
endif()

set(QwertycoinTools_Daemon_LIBS
    Boost::filesystem
    QwertycoinFramework::Breakpad
    QwertycoinFramework::Common
    QwertycoinFramework::CryptoNoteCore
    QwertycoinFramework::Global
    QwertycoinFramework::Logging
    QwertycoinFramework::P2p
    QwertycoinFramework::Rpc
    QwertycoinFramework::Serialization
    QwertycoinFramework::System
)

add_executable(QwertycoinTools_Daemon ${QwertycoinTools_Daemon_SOURCES})
add_executable(QwertycoinTools::Daemon ALIAS QwertycoinTools_Daemon)
target_include_directories(QwertycoinTools_Daemon PRIVATE ${QwertycoinTools_INCLUDE_DIRS})
target_link_libraries(QwertycoinTools_Daemon PRIVATE ${QwertycoinTools_Daemon_LIBS})
set_target_properties(QwertycoinTools_Daemon PROPERTIES OUTPUT_NAME "qwertycoind")

# QwertycoinTools::PaymentGateService

set(QwertycoinTools_PaymentGateService_SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/PaymentGateService/ConfigurationManager.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/PaymentGateService/ConfigurationManager.h"
    "${CMAKE_CURRENT_LIST_DIR}/PaymentGateService/PaymentGateService.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/PaymentGateService/PaymentGateService.h"
    "${CMAKE_CURRENT_LIST_DIR}/PaymentGateService/PaymentServiceConfiguration.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/PaymentGateService/PaymentServiceConfiguration.h"
    "${CMAKE_CURRENT_LIST_DIR}/PaymentGateService/RpcNodeConfiguration.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/PaymentGateService/RpcNodeConfiguration.h"
    "${CMAKE_CURRENT_LIST_DIR}/PaymentGateService/main.cpp"
)

if(WIN32)
    list(APPEND QwertycoinTools_PaymentGateService_SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/BinaryInfo/walletd.rc"
    )
endif()

set(QwertycoinTools_PaymentGateService_LIBS
    Boost::filesystem
    Boost::program_options
    QwertycoinFramework::Breakpad
    QwertycoinFramework::Common
    QwertycoinFramework::CryptoNoteCore
    QwertycoinFramework::Global
    QwertycoinFramework::InProcessNode
    QwertycoinFramework::Logging
    QwertycoinFramework::Rpc
    QwertycoinFramework::System
)

add_executable(QwertycoinTools_PaymentGateService ${QwertycoinTools_PaymentGateService_SOURCES})
add_executable(QwertycoinTools::PaymentGateService ALIAS QwertycoinTools_PaymentGateService)
target_include_directories(QwertycoinTools_PaymentGateService PRIVATE ${QwertycoinTools_INCLUDE_DIRS})
target_link_libraries(QwertycoinTools_PaymentGateService ${QwertycoinTools_PaymentGateService_LIBS})
set_target_properties(QwertycoinTools_PaymentGateService PROPERTIES OUTPUT_NAME "walletd")

# QwertycoinTools::SimpleWallet

set(QwertycoinTools_SimpleWallet_SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/SimpleWallet/PasswordContainer.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/SimpleWallet/PasswordContainer.h"
    "${CMAKE_CURRENT_LIST_DIR}/SimpleWallet/SimpleWallet.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/SimpleWallet/SimpleWallet.h"
)

if(WIN32)
    list(APPEND QwertycoinTools_SimpleWallet_SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/BinaryInfo/simplewallet.rc"
    )
endif()

set(QwertycoinTools_SimpleWallet_LIBS
    Boost::filesystem
    Boost::program_options
    QwertycoinFramework::Breakpad
    QwertycoinFramework::Common
    QwertycoinFramework::Global
    QwertycoinFramework::Logging
    QwertycoinFramework::NodeRpcProxy
    QwertycoinFramework::Rpc
    QwertycoinFramework::Serialization
    QwertycoinFramework::Wallet
)

if(WIN32)
    list(APPEND QwertycoinTools_SimpleWallet_LIBS ws2_32)
endif()

add_executable(QwertycoinTools_SimpleWallet ${QwertycoinTools_SimpleWallet_SOURCES})
add_executable(QwertycoinTools::SimpleWallet ALIAS QwertycoinTools_SimpleWallet)
target_include_directories(QwertycoinTools_SimpleWallet PRIVATE ${QwertycoinTools_INCLUDE_DIRS})
target_link_libraries(QwertycoinTools_SimpleWallet ${QwertycoinTools_SimpleWallet_LIBS})
set_target_properties(QwertycoinTools_SimpleWallet PROPERTIES OUTPUT_NAME "simplewallet")

# Installation

install(TARGETS
            QwertycoinTools_Daemon
            QwertycoinTools_PaymentGateService
            QwertycoinTools_SimpleWallet
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib/static
)
