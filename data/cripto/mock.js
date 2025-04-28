// retirar antes da versão final
// Interceptar chamadas fetch
const originalFetch = window.fetch;
window.fetch = async (url, options) => {
    console.log(`[mock] fetch para: ${url}`);
    
    if (url === '/scan-wifi') {
        await new Promise(resolve => setTimeout(resolve, 2000));
        return new Response(JSON.stringify([
            { 
                ssid: 'sideout', 
                rssi: -42,
                authMode: 'WPA2', // ou isOpen: false
                open: false
            },
            { 
                ssid: 'guest_wifi', 
                rssi: -70,
                authMode: 'WPA', // ou isOpen: false
                open: false
            },
            { 
                ssid: 'guest', 
                rssi: -70,
                authMode: 'OPEN', // ou isOpen: true
                open: true
            }
        ]), {
            status: 200,
            headers: { 'Content-Type': 'application/json' }
        });
    }
    
    if (url === '/wifi-settings') {
        // Alterna entre true e false para simular diferentes estados
        const dhcpState = Math.random() > 0.5;
        
        return new Response(JSON.stringify({
            dhcp: dhcpState,
            mDns: "modulo1",
            ip: dhcpState ? "" : "192.168.0.193", // IP vazio se DHCP estiver ativo
            gateway: dhcpState ? "" : "192.168.0.1",
            subnet: dhcpState ? "" : "255.255.255.0"
        }), {
            status: 200,
            headers: { 'Content-Type': 'application/json' }
        });
    }

    if (url === '/connect-wifi') {
        console.log(`[mock] Conectando à rede: ${JSON.parse(options.body).ssid}`);
        console.log(`[mock] Senha: ${JSON.parse(options.body).password}`);
        console.log(`[mock] DHCP: ${JSON.parse(options.body).dhcp}`);
        console.log(`[mock] mDNS: ${JSON.parse(options.body).mDns}`);

        await new Promise(resolve => setTimeout(resolve, 2000));
        
        return new Response(JSON.stringify({
            message: 'Conectado com sucesso à rede ' + JSON.parse(options.body).ssid
        }), {
            status: 200,
            headers: { 'Content-Type': 'application/json' }
        });
    }

    if (url === '/save-wifi-settings') {
        console.log(`[mock] Salvando configurações de WiFi:`);
    
        const body = JSON.parse(options.body);
    
        console.log(`[mock] Payload criptografado:`, body.data);
    
        // Simula o servidor aceitando sem descriptografar (porque no mock você ainda não tem a descriptografia)
    
        await new Promise(resolve => setTimeout(resolve, 2000));
    
        return new Response(JSON.stringify({
            message: 'Configuração salva com sucesso!'
        }), {
            status: 200,
            headers: { 'Content-Type': 'application/json' }
        });
    }

    if (url === '/save-wifi-credentials') {
        console.log(`[mock] Salvando configurações de WiFi: ${JSON.parse(options.body).ssid}`);
        return new Response(JSON.stringify({
            message: 'Configurações salvas com sucesso!'
        }), {
            status: 200,
            headers: { 'Content-Type': 'application/json' }
        });
    }

    // Fallback: chamada real
    return originalFetch(url, options);
};
// retirar antes da versão final
