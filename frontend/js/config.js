/* Backend configuration.
 * Local desktop/browser development uses the local C++ backend.
 * Hosted deployments use same-origin /api requests routed by Vercel
 * to the Render backend.
 */
(function () {
  'use strict';

  const isLocalRuntime =
    window.location.protocol === 'file:' ||
    window.location.hostname === 'localhost' ||
    window.location.hostname === '127.0.0.1';

  window.DLX_CONFIG = {
    apiBaseUrl: isLocalRuntime ? 'http://127.0.0.1:8080' : '',
    requestTimeoutMs: 30000,
  };
})();
