/*
 * Offline preview fixtures only: every name, address, log line, and version
 * below is fabricated. To refresh after a UI change, recopy common/app/html/index.html
 * here, keep the mock.js include before its inline app script, then update these fixtures.
 */
(() => {
    'use strict';

    const settings = {
        CustomServer: '',
        AuthKey: '',
        HttpProxyPort: '8080',
        Socks5Port: '1080',
        AcceptDNS: 'true',
        AcceptRoutes: 'false',
        AdvertiseRoutes: '',
        ExitNode: '',
        ExitNodeAllowLAN: 'true',
        SendLogs: 'false'
    };
    const fakeStatus = {
        BackendState: 'Running',
        Version: '1.80.0',
        Self: {
            Online: true,
            HostName: 'preview-node.invalid',
            TailscaleIPs: ['100.64.0.42']
        },
        CurrentTailnet: { Name: 'sample-tailnet.invalid' }
    };
    const fakeLog = [
        '2026-01-15T12:00:00+00:00 preview-node.invalid [100] Program starting: v1.80.0',
        'Jan 15 12:00:01 preview-node.invalid tailscaled[100]: Switching ipn state NoState -> Running',
        'Jan 15 12:00:02 preview-node.invalid tailscaled[100]: peerapi: serving on http://100.64.0.42:41437',
        'Jan 15 12:00:03 preview-node.invalid tailscaled[100]: active login: sample-user@example.invalid',
        'Jan 15 12:00:04 preview-node.invalid tailscaled[100]: HTTP/HTTPS proxy: http://127.0.0.1:8080',
        'Jan 15 12:00:04 preview-node.invalid tailscaled[100]: SOCKS5 proxy: 127.0.0.1:1080',
        'Jan 15 12:00:05 preview-node.invalid tailscaled[100]: Tailscale VPN is running'
    ].join('\n');

    const preview = window.__offlinePreview = {
        requests: [],
        blockedExternalRequests: [],
        settings
    };

    function response(body, contentType, status) {
        return new Response(body, {
            status: status || 200,
            headers: { 'Content-Type': contentType || 'application/json' }
        });
    }

    function json(value) {
        return response(JSON.stringify(value));
    }

    function parseForm(body) {
        return Object.fromEntries(new URLSearchParams(body || ''));
    }

    window.fetch = async (input, init = {}) => {
        const address = typeof input === 'string' || input instanceof URL
            ? String(input)
            : input.url;
        const url = new URL(address, window.location.href);
        const method = (init.method || (input instanceof Request ? input.method : 'GET')).toUpperCase();

        if (url.origin !== window.location.origin) {
            preview.blockedExternalRequests.push(url.href);
            if (url.hostname === 'api.github.com') {
                return json({ tag_name: 'v1.80.0', html_url: 'https://example.invalid/preview-release' });
            }
            return response('External requests are disabled in this offline preview.', 'text/plain', 451);
        }

        preview.requests.push({ method, path: url.pathname });

        if (url.pathname.endsWith('/status.json')) return json(fakeStatus);
        if (url.pathname === '/axis-cgi/applications/list.cgi') {
            return response('<root><application Name="Tailscale_VPN" Status="Running"/></root>', 'application/xml');
        }
        if (url.pathname === '/axis-cgi/admin/systemlog.cgi') return response(fakeLog, 'text/plain');
        if (url.pathname === '/axis-cgi/param.cgi' && method === 'GET') {
            const fields = Object.entries(settings).map(([key, value]) => `root.Tailscale_VPN.${key}=${value}`);
            return response(fields.join('\n'), 'text/plain');
        }
        if (url.pathname === '/axis-cgi/param.cgi' && method === 'POST') {
            Object.assign(settings, parseForm(init.body));
            Object.keys(settings).forEach(key => {
                const submitted = settings[`root.Tailscale_VPN.${key}`];
                if (submitted !== undefined) settings[key] = submitted;
            });
            return response('OK', 'text/plain');
        }
        if (url.pathname === '/axis-cgi/applications/control.cgi' && method === 'POST') {
            return response('OK (preview only)', 'text/plain');
        }
        if (url.pathname === '/local/Tailscale_VPN/api/settings') {
            if (method === 'POST') {
                Object.assign(settings, parseForm(init.body));
                return response('OK', 'text/plain');
            }
            return json(settings);
        }
        return response('Not found in offline preview.', 'text/plain', 404);
    };

    const notice = document.createElement('div');
    notice.textContent = 'OFFLINE PREVIEW · FICTIONAL SAMPLE DATA · CHANGES STAY IN THIS PAGE';
    notice.setAttribute('role', 'note');
    notice.style.cssText = 'background:#fff3cd;color:#5a4300;border:1px solid #e8cf83;border-radius:6px;padding:8px 12px;margin:0 0 14px;font:600 11px/1.4 -apple-system,BlinkMacSystemFont,"Segoe UI",sans-serif;letter-spacing:.03em;text-align:center;';
    document.body.prepend(notice);

    document.addEventListener('click', event => {
        const link = event.target.closest('a[href]');
        if (!link) return;
        const destination = new URL(link.href, window.location.href);
        if (destination.origin !== window.location.origin) {
            event.preventDefault();
            event.stopImmediatePropagation();
        }
    }, true);
})();
