/* Where the C++ solver backend lives. Loaded before solver-client.js.
 *
 * apiBaseUrl      Origin of the backend, without a trailing slash.
 * requestTimeoutMs  How long a solve may take before the UI gives up.
 *
 * If you change apiBaseUrl to another origin, also add it to the `connect-src` list of the
 * Content-Security-Policy in index.html, and make sure the backend allows this page's origin
 * (DLX_CORS_ORIGINS in the backend's environment). See README.md. */
window.DLX_CONFIG = {
  apiBaseUrl: 'http://127.0.0.1:8080',
  requestTimeoutMs: 30000,
};
