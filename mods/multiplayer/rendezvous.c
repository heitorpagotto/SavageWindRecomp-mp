#include "rendezvous.h"
#include "random.h"

#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TOKEN_SIZE 32u
#define INVITE_SIZE 24u
#define BINDING_HEX (2u * SWMP_BINDING_SIZE)
#define POLL_MS UINT64_C(350)

typedef enum {
    OFFLINE,
    REGISTER_HOST,
    GET_OFFER,
    GATHER_ANSWER,
    PUT_ANSWER,
    GATHER_OFFER,
    PUT_OFFER,
    GET_ANSWER,
    ANSWER_RECEIVED,
} Phase;

struct SwmpRendezvous {
    CURLM *multi;
    CURL *request;
    struct curl_slist *headers;
    const char *ca_bundle;
    SwmpWebrtc *rtc;
    Phase phase;
    SwmpConnection link;
    uint64_t poll_after;
    char origin[256];
    char invite[INVITE_SIZE + 1];
    char binding[BINDING_HEX + 1];
    char body[INVITE_SIZE + 1 + BINDING_HEX + 1 + SWMP_WEBRTC_SDP_SIZE];
    char response[BINDING_HEX + 1 + SWMP_WEBRTC_SDP_SIZE];
    size_t reply_size;
};

static void hex_encode(char *text, const uint8_t *bytes, size_t size) {
    static const char digits[] = "0123456789abcdef";
    for (size_t i = 0; i < size; i++) {
        text[2 * i] = digits[bytes[i] >> 4];
        text[2 * i + 1] = digits[bytes[i] & 15];
    }
    text[2 * size] = 0;
}

static bool hex_decode(uint8_t out[SWMP_BINDING_SIZE], const char *text) {
    static const char digits[] = "0123456789abcdef";
    for (unsigned i = 0; i < SWMP_BINDING_SIZE; i++) {
        if (!text[2 * i] || !text[2 * i + 1]) return false;
        const char *hi = strchr(digits, text[2 * i]), *lo = strchr(digits, text[2 * i + 1]);
        if (!hi || !lo) return false;
        out[i] = (uint8_t)(((hi - digits) << 4) | (lo - digits));
    }
    return true;
}

static bool valid_invite(const char *invite) {
    static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
    return strlen(invite) == INVITE_SIZE && strspn(invite, alphabet) == INVITE_SIZE;
}

static bool valid_origin(const char *url) {
    static const char prefix[] = "https://";
    static const char authority[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz.:-[]";
    if (!url || strncmp(url, prefix, sizeof prefix - 1)) return false;
    const char *host = url + sizeof prefix - 1;
    size_t n = strspn(host, authority);
    return n && (host[n] == 0 || (host[n] == '/' && host[n + 1] == 0));
}

static void request_close(SwmpRendezvous *s) {
    if (!s->request) return;
    curl_multi_remove_handle(s->multi, s->request);
    curl_easy_cleanup(s->request);
    s->request = NULL;
}

static void stop(void *context) {
    SwmpRendezvous *s = context;
    request_close(s);
    curl_slist_free_all(s->headers);
    s->headers = NULL;
    if (s->multi) curl_multi_cleanup(s->multi);
    s->multi = NULL;
    s->rtc = NULL;
    s->phase = OFFLINE;
    s->link = 0;
    s->poll_after = 0;
    s->origin[0] = s->invite[0] = s->binding[0] = s->body[0] = s->response[0] = 0;
}

SwmpRendezvous *swmp_rendezvous_create(const char *ca_bundle) {
    const curl_version_info_data *version = curl_version_info(CURLVERSION_NOW);
    if (!(version->features & CURL_VERSION_ASYNCHDNS) || !(version->features & CURL_VERSION_SSL))
        return NULL;
    SwmpRendezvous *s = calloc(1, sizeof *s);
    if (!s) return NULL;
    s->ca_bundle = ca_bundle;
    return s;
}

void swmp_rendezvous_destroy(SwmpRendezvous *s) {
    if (!s) return;
    stop(s);
    free(s);
}

static bool begin(void *context, SwmpWebrtc *rtc, const char *url, const char *invite,
                  char *error, size_t error_size) {
    SwmpRendezvous *s = context;
    if (!valid_origin(url) || strlen(url) >= sizeof s->origin) {
        snprintf(error, error_size, "Set a valid HTTPS rendezvous origin");
        return false;
    }
    if (invite && !valid_invite(invite)) {
        snprintf(error, error_size, "Invalid lobby invite");
        return false;
    }

    uint8_t token[TOKEN_SIZE];
    uint8_t binding[SWMP_BINDING_SIZE];
    if (!swmp_random_bytes(token, sizeof token) || (invite && !swmp_random_bytes(binding, sizeof binding))) {
        snprintf(error, error_size, "Could not generate lobby credentials");
        return false;
    }
    char auth[sizeof "Authorization: Bearer " + 2 * TOKEN_SIZE] = "Authorization: Bearer ";
    hex_encode(auth + sizeof "Authorization: Bearer " - 1, token, sizeof token);
    s->headers = curl_slist_append(NULL, auth);
    s->multi = curl_multi_init();
    if (!s->headers || !s->multi) {
        snprintf(error, error_size, "Could not initialize lobby signaling");
        return false;
    }

    strcpy(s->origin, url);
    size_t end = strlen(s->origin) - 1;
    if (s->origin[end] == '/') s->origin[end] = 0;
    if (invite) {
        strcpy(s->invite, invite);
        hex_encode(s->binding, binding, sizeof binding);
    }
    s->rtc = rtc;
    s->phase = invite ? GATHER_OFFER : REGISTER_HOST;
    return true;
}

SwmpWebrtcSignaling swmp_rendezvous_signaling(SwmpRendezvous *s) {
    return (SwmpWebrtcSignaling){.context = s, .begin = begin, .stop = stop};
}

static size_t receive(char *data, size_t size, size_t count, void *context) {
    SwmpRendezvous *s = context;
    size_t bytes = size * count;  // libcurl supplies size 1 for this callback
    if (bytes >= sizeof s->response - s->reply_size || memchr(data, 0, bytes)) return 0;
    memcpy(s->response + s->reply_size, data, bytes);
    s->reply_size += bytes;
    s->response[s->reply_size] = 0;
    return bytes;
}

static bool start_request(SwmpRendezvous *s, const char *path, bool post) {
    char url[sizeof s->origin + 32];
    snprintf(url, sizeof url, "%s/v1/%s", s->origin, path);
    s->reply_size = 0;
    s->response[0] = 0;
    CURL *c = s->request = curl_easy_init();
    if (!c) return false;

    // No redirect may receive the bearer credential. libcurl checks the peer and the hostname.
    return curl_easy_setopt(c, CURLOPT_URL, url) == CURLE_OK &&
           curl_easy_setopt(c, CURLOPT_PROTOCOLS_STR, "https") == CURLE_OK &&
           curl_easy_setopt(c, CURLOPT_FOLLOWLOCATION, 0L) == CURLE_OK &&
           curl_easy_setopt(c, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2) == CURLE_OK &&
           (!s->ca_bundle || curl_easy_setopt(c, CURLOPT_CAINFO, s->ca_bundle) == CURLE_OK) &&
           curl_easy_setopt(c, CURLOPT_HTTPHEADER, s->headers) == CURLE_OK &&
           curl_easy_setopt(c, CURLOPT_TIMEOUT_MS, 8000L) == CURLE_OK &&
           curl_easy_setopt(c, CURLOPT_CONNECTTIMEOUT_MS, 4000L) == CURLE_OK &&
           curl_easy_setopt(c, CURLOPT_NOSIGNAL, 1L) == CURLE_OK &&
           curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, receive) == CURLE_OK &&
           curl_easy_setopt(c, CURLOPT_WRITEDATA, s) == CURLE_OK &&
           (!post || curl_easy_setopt(c, CURLOPT_POSTFIELDS, s->body) == CURLE_OK) &&
           curl_multi_add_handle(s->multi, c) == CURLM_OK;
}

static void failed(SwmpRendezvous *s, const char *error) {
    request_close(s);
    s->phase = OFFLINE;
    swmp_webrtc_signal_error(s->rtc, error);
}

static void response(SwmpRendezvous *s, long status, uint64_t now) {
    char *sdp = s->response + BINDING_HEX + 1;
    uint8_t binding[SWMP_BINDING_SIZE];
    bool has_sdp = s->reply_size > BINDING_HEX + 1 && s->response[BINDING_HEX] == '\n' &&
                   hex_decode(binding, s->response);

    switch (s->phase) {
    case REGISTER_HOST:
        if (status == 200 && valid_invite(s->response) && swmp_webrtc_set_invite(s->rtc, s->response)) {
            s->phase = GET_OFFER;
            return;
        }
        break;
    case GET_OFFER:
        if (status == 204) {
            s->poll_after = now + POLL_MS;
            return;
        }
        if (status == 200 && has_sdp) {
            s->link = swmp_webrtc_open(s->rtc, binding, sdp, now);
            hex_encode(s->binding, binding, sizeof binding);
            s->phase = GATHER_ANSWER;
            return;
        }
        break;
    case PUT_ANSWER:
        if (status == 204 || status == 404) {
            if (status == 404 && s->link) {
                SwmpTransport t = swmp_webrtc_transport(s->rtc);
                t.disconnect(t.context, s->link);
            }
            s->link = 0;
            s->phase = GET_OFFER;
            return;
        }
        break;
    case PUT_OFFER:
        if (status == 204) {
            s->phase = GET_ANSWER;
            return;
        }
        if (status == 404) {
            failed(s, "Lobby invite is unknown or expired");
            return;
        }
        break;
    case GET_ANSWER:
        if (status == 204) {
            s->poll_after = now + POLL_MS;
            return;
        }
        if (status == 200 && has_sdp && !memcmp(s->binding, s->response, BINDING_HEX) &&
            swmp_webrtc_set_answer(s->rtc, s->link, sdp)) {
            s->phase = ANSWER_RECEIVED;
            return;
        }
        if (status == 404 || status == 409) {
            failed(s, "Host could not accept this connection");
            return;
        }
        break;
    default: break;
    }
    failed(s, status == 503 ? "Rendezvous service is full" : "Unexpected rendezvous response");
}

static bool gather_offer(SwmpRendezvous *s, uint64_t now) {
    if (!s->link) {
        uint8_t binding[SWMP_BINDING_SIZE];
        if (!hex_decode(binding, s->binding)) return false;
        s->link = swmp_webrtc_open(s->rtc, binding, NULL, now);
        if (!s->link) return false;
    }
    int size = snprintf(s->body, sizeof s->body, "%s\n%s\n", s->invite, s->binding);
    int result = swmp_webrtc_local_sdp(s->rtc, s->link, s->body + size);
    if (result <= 0) return result == 0;
    s->phase = PUT_OFFER;
    return start_request(s, "join", true);
}

static bool gather_answer(SwmpRendezvous *s) {
    int size = snprintf(s->body, sizeof s->body, "%s\n", s->binding);
    int result = s->link ? swmp_webrtc_local_sdp(s->rtc, s->link, s->body + size) : -1;
    if (!result) return true;
    if (result < 0) {
        s->body[size] = 0;  // decline, then consume the next offer
        if (s->link) {
            SwmpTransport t = swmp_webrtc_transport(s->rtc);
            t.disconnect(t.context, s->link);
            s->link = 0;
        }
    }
    s->phase = PUT_ANSWER;
    return start_request(s, "answer", true);
}

void swmp_rendezvous_update(SwmpRendezvous *s, uint64_t now) {
    if (s->phase == OFFLINE) return;
    swmp_webrtc_expire(s->rtc, now);
    if (s->request) {
        int running, left;
        if (curl_multi_perform(s->multi, &running) != CURLM_OK) {
            failed(s, "Lobby signaling failed");
            return;
        }
        CURLMsg *done = curl_multi_info_read(s->multi, &left);
        if (!done || done->msg != CURLMSG_DONE) return;
        long status = 0;
        CURLcode result = done->data.result;
        curl_easy_getinfo(s->request, CURLINFO_RESPONSE_CODE, &status);
        request_close(s);
        if (result != CURLE_OK) failed(s, "Could not reach the HTTPS rendezvous service");
        else response(s, status, now);
        return;
    }

    if (now < s->poll_after) return;
    bool ok = true;
    switch (s->phase) {
    case REGISTER_HOST: ok = start_request(s, "room", true); break;
    case GET_OFFER: ok = start_request(s, "offer", false); break;
    case GATHER_ANSWER: ok = gather_answer(s); break;
    case GATHER_OFFER: ok = gather_offer(s, now); break;
    case GET_ANSWER: ok = start_request(s, "answer", false); break;
    default: break;
    }
    if (!ok) failed(s, "Could not prepare the lobby connection");
}
