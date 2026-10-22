#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/provider.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <openssl/ssl.h>
#include <openssl/x509v3.h>
#include <array>
#include <cstring>
#include <dlfcn.h>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace {
void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
template<class Type, auto Free> using Owned = std::unique_ptr<Type, decltype(Free)>;
std::string hex(const unsigned char* bytes, std::size_t size) {
    constexpr char alphabet[] = "0123456789abcdef";
    std::string result;
    for (std::size_t i = 0; i < size; ++i) { result += alphabet[bytes[i] >> 4]; result += alphabet[bytes[i] & 15]; }
    return result;
}
template<class Function>
void loaded_from(Function symbol, const std::filesystem::path& expected) {
    Dl_info info{};
    require(dladdr(reinterpret_cast<const void*>(symbol), &info) != 0 && info.dli_fname,
            "Cannot identify the loaded OpenSSL image");
    require(std::filesystem::equivalent(info.dli_fname, expected), "OpenSSL loaded outside the expected bundle");
    std::cout << "loaded " << info.dli_fname << '\n';
}
void hashes_and_providers() {
    std::array<unsigned char, EVP_MAX_MD_SIZE> output{};
    unsigned size{};
    require(EVP_Digest("abc", 3, output.data(), &size, EVP_sha256(), nullptr) == 1 && size == 32 &&
        hex(output.data(), size) == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
        "The actual SHA256 provider does not match the known vector");
    SHA1(reinterpret_cast<const unsigned char*>("abc"), 3, output.data());
    require(hex(output.data(), 20) == "a9993e364706816aba3e25717850c26c9cd0d89d", "The actual SHA1 API differs");
    std::array<unsigned char, 45> base64{};
    require(EVP_EncodeBlock(base64.data(), reinterpret_cast<const unsigned char*>("abc"), 3) == 4 &&
        std::string(reinterpret_cast<const char*>(base64.data())) == "YWJj", "The actual base64 API differs");
    std::array<unsigned char, 32> first{}, second{};
    require(RAND_status() == 1 && RAND_bytes(first.data(), first.size()) == 1 &&
        RAND_bytes(second.data(), second.size()) == 1 && first != second,
        "The actual random generator failed or repeated its security value");
    Owned<OSSL_PROVIDER, OSSL_PROVIDER_unload> legacy(OSSL_PROVIDER_load(nullptr, "legacy"), OSSL_PROVIDER_unload);
    require(bool(legacy), "The upstream legacy provider was not retained as a built-in provider");
    Owned<EVP_MD, EVP_MD_free> md4(EVP_MD_fetch(nullptr, "MD4", "provider=legacy"), EVP_MD_free);
    require(bool(md4), "The retained built-in provider cannot supply its real algorithm");
}
void tls() {
    Owned<EVP_PKEY, EVP_PKEY_free> key(EVP_PKEY_Q_keygen(nullptr, nullptr, "EC", "prime256v1"), EVP_PKEY_free);
    Owned<X509, X509_free> certificate(X509_new(), X509_free);
    require(key && certificate && X509_set_version(certificate.get(), 2) == 1 &&
        ASN1_INTEGER_set(X509_get_serialNumber(certificate.get()), 1) == 1 &&
        X509_gmtime_adj(X509_getm_notBefore(certificate.get()), -3600) &&
        X509_gmtime_adj(X509_getm_notAfter(certificate.get()), 3600) &&
        X509_set_pubkey(certificate.get(), key.get()) == 1, "Cannot create the private in-memory TLS certificate");
    auto* subject = X509_get_subject_name(certificate.get());
    require(X509_NAME_add_entry_by_txt(subject, "CN", MBSTRING_ASC,
        reinterpret_cast<const unsigned char*>("halo2-test.local"), -1, -1, 0) == 1 &&
        X509_set_issuer_name(certificate.get(), subject) == 1, "Cannot set the test certificate identity");
    Owned<X509_EXTENSION, X509_EXTENSION_free> alternative(
        X509V3_EXT_conf_nid(nullptr, nullptr, NID_subject_alt_name, const_cast<char*>("DNS:halo2-test.local")), X509_EXTENSION_free);
    require(alternative && X509_add_ext(certificate.get(), alternative.get(), -1) == 1 &&
        X509_sign(certificate.get(), key.get(), EVP_sha256()) > 0, "Cannot sign the private test certificate");
    Owned<SSL_CTX, SSL_CTX_free> server_context(SSL_CTX_new(TLS_server_method()), SSL_CTX_free);
    Owned<SSL_CTX, SSL_CTX_free> client_context(SSL_CTX_new(TLS_client_method()), SSL_CTX_free);
    require(server_context && client_context &&
        SSL_CTX_set_min_proto_version(server_context.get(), TLS1_3_VERSION) == 1 &&
        SSL_CTX_set_min_proto_version(client_context.get(), TLS1_3_VERSION) == 1 &&
        SSL_CTX_use_certificate(server_context.get(), certificate.get()) == 1 &&
        SSL_CTX_use_PrivateKey(server_context.get(), key.get()) == 1 &&
        X509_STORE_add_cert(SSL_CTX_get_cert_store(client_context.get()), certificate.get()) == 1,
        "Cannot initialize the real TLS contexts/trust store");
    SSL_CTX_set_verify(client_context.get(), SSL_VERIFY_PEER, nullptr);
    for (bool trusted_hostname : {true, false}) {
        Owned<SSL, SSL_free> client(SSL_new(client_context.get()), SSL_free), server(SSL_new(server_context.get()), SSL_free);
        BIO *client_bio{}, *server_bio{};
        require(client && server && BIO_new_bio_pair(&client_bio, 0, &server_bio, 0) == 1,
            "Cannot create the TLS memory transport");
        SSL_set_bio(client.get(), client_bio, client_bio); SSL_set_bio(server.get(), server_bio, server_bio);
        SSL_set_connect_state(client.get()); SSL_set_accept_state(server.get());
        require(SSL_set1_host(client.get(), trusted_hostname ? "halo2-test.local" : "wrong-test.local") == 1,
            "Cannot set the TLS hostname constraint");
        bool rejected = false;
        for (unsigned iteration = 0; iteration < 1024 &&
            (!SSL_is_init_finished(client.get()) || !SSL_is_init_finished(server.get())); ++iteration) {
            for (SSL* peer : {client.get(), server.get()}) {
                const auto result = SSL_do_handshake(peer);
                if (result == 1) continue;
                const auto error = SSL_get_error(peer, result);
                if (error == SSL_ERROR_WANT_READ || error == SSL_ERROR_WANT_WRITE) continue;
                if (!trusted_hostname && peer == client.get() &&
                    SSL_get_verify_result(client.get()) == X509_V_ERR_HOSTNAME_MISMATCH) { rejected = true; break; }
                throw std::runtime_error("The actual TLS handshake failed unexpectedly");
            }
            if (rejected) break;
        }
        if (!trusted_hostname) { require(rejected, "The real TLS verifier admitted a mismatched hostname"); continue; }
        require(SSL_is_init_finished(client.get()) && SSL_is_init_finished(server.get()) &&
            SSL_get_verify_result(client.get()) == X509_V_OK && SSL_version(client.get()) == TLS1_3_VERSION,
            "The certificate-verified TLS 1.3 handshake did not complete");
        const std::string plaintext = "Halo 2 universal dependency transport";
        std::array<char, 128> received{}; std::size_t written{}, count{};
        require(SSL_write_ex(client.get(), plaintext.data(), plaintext.size(), &written) == 1 && written == plaintext.size() &&
            SSL_read_ex(server.get(), received.data(), received.size(), &count) == 1 &&
            std::string(received.data(), count) == plaintext, "The real TLS application record did not round trip");
    }
}
}
int main(int argc, char** argv) try {
    require(argc == 2, "Supply the actual bundle Frameworks directory");
    loaded_from(&EVP_Digest, std::filesystem::path(argv[1]) / "libcrypto.3.dylib");
    loaded_from(&SSL_new, std::filesystem::path(argv[1]) / "libssl.3.dylib");
    require(OPENSSL_version_major() == 3 && OPENSSL_version_minor() == 5 && OPENSSL_version_patch() == 9,
        "The loaded OpenSSL version differs from the pinned source");
    hashes_and_providers(); tls();
    std::cout << "PASS " << OpenSSL_version(OPENSSL_VERSION)
        << ": SHA1/SHA256/base64/random, built-in legacy provider, certificate-verified TLS 1.3 record and hostname rejection\n";
    return 0;
} catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
