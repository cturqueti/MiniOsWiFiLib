// =============================================
// 1. CONSTANTES E ELEMENTOS DA INTERFACE
// =============================================
const modal = {
    element: document.getElementById('modal'),
    title: document.getElementById('modal-title'),
    message: document.getElementById('modal-message'),
    closeBtn: document.getElementById('modal-close'),
    saveBtn: document.getElementById('save-wifi-credentials-btn')
};

const wifiForm = {
    ssidSelect: document.getElementById('ssid'),
    passwordField: document.getElementById('password-field'),
    passwordInput: document.getElementById('password'),
    refreshBtn: document.getElementById('refresh-wifi'),
};

const staticIpForm = {
    dhcpCheckbox: document.getElementById('dhcp-checkbox'),
    mDnsInput:  document.getElementById('mDns'),
    ipFields: document.querySelectorAll('.ip-group input[type="number"]'),
    saveConfigBtn: document.getElementById('save-wifi-config-btn')
};

// =============================================
// 2. FUNÇÕES UTILITÁRIAS
// =============================================
// Controle do Modal
function showModal(title, message) {
    modal.title.textContent = title;
    modal.message.textContent = message;
    modal.element.classList.add('active');
}

function closeModal() {
    modal.element.classList.remove('active');
}

// Estados de carregamento
// Função setLoading atualizada para trabalhar com seu botão específico
function setLoading(state) {
    const buttons = [staticIpForm.saveConfigBtn, wifiForm.refreshBtn];
    
    buttons.forEach(btn => {
        if (!btn) return;

        if (btn.id === 'refresh-wifi') {
            // Comportamento para o botão de refresh (rotação SVG)
            if (state) {
                btn.disabled = true;
                btn.classList.add('loading');
                const svg = btn.querySelector('svg');
                if (svg) svg.style.transform = 'rotate(360deg)';
            } else {
                btn.disabled = false;
                btn.classList.remove('loading');
                const svg = btn.querySelector('svg');
                if (svg) svg.style.transform = 'rotate(0deg)';
            }
        } 
    });
}

// Validação de rede
function validateNetworkSelection(ssid, isOpenNetwork, password) {
    if (!ssid) {
        showModal('Erro', 'Selecione uma rede Wi-Fi');
        return false;
    }
    if (!isOpenNetwork && !password) {
        showModal('Erro', 'Digite a senha da rede Wi-Fi');
        return false;
    }
    return true;
}

// Validação de mDNS
function validateMDns(mDns) {
    if (!mDns || !/^[a-zA-Z0-9]+$/.test(mDns)) {
        showModal('Erro', 'Nome do dispositivo inválido! Deve conter apenas letras e números.');
        return false;
    }
    return true;
}

// Validação de IP
function isValidOctet(value) {
    return value !== '' && !isNaN(value) && value >= 0 && value <= 255;
}

function isNotFirstOctetZero(octet) {
    return octet != 0;
}

function isNotLastOctetZeroOr255(octet) {
    return octet != 0 && octet != 255;
}

function isValidIP(ip1, ip2, ip3, ip4) {
    if (![ip1, ip2, ip3, ip4].every(isValidOctet)) return false;
    if (!isNotFirstOctetZero(ip1)) return false;
    if (!isNotLastOctetZeroOr255(ip4)) return false;
    if (ip1 == 127 && ip2 == 0 && ip3 == 0 && ip4 == 1) return false;
    if (ip1 == 255 && ip2 == 255 && ip3 == 255 && ip4 == 255) return false;
    return true;
}

function isValidSubnet(sn1, sn2, sn3, sn4) {
    if (![sn1, sn2, sn3, sn4].every(isValidOctet)) return false;
    
    const mask = (sn1 << 24) | (sn2 << 16) | (sn3 << 8) | sn4;
    if (mask === 0 || mask === 0xFFFFFFFF) return false;
    
    let foundZero = false;
    for (let i = 31; i >= 0; i--) {
        const bit = (mask >> i) & 1;
        if (bit === 0) foundZero = true;
        if (foundZero && bit === 1) return false;
    }
    return true;
}

function areInSameNetwork(ip, gateway, subnet) {
    const ipParts = ip.split('.').map(Number);
    const gwParts = gateway.split('.').map(Number);
    const snParts = subnet.split('.').map(Number);
    
    return ipParts.every((part, i) => (part & snParts[i]) === (gwParts[i] & snParts[i]));
}

// =============================================
// 3. FUNÇÕES PRINCIPAIS
// =============================================
// Função para buscar redes WiFi
async function scanWifi() {
    
    wifiForm.ssidSelect.innerHTML = '<option value="">Selecione uma rede...</option>';

    try {
        setLoading(true);
        const response = await fetch('/scan-wifi');
        if (!response.ok) throw new Error(`Erro HTTP! status: ${response.status}`);

        const networks = await response.json();

        networks.forEach(network => {
            const option = document.createElement('option');
            option.value = network.ssid;
            const lockIcon = network.open ? '🔓' : '🔒';
            const authStatus = network.open ? ' (Aberta)' : ' (Protegida)';
            option.textContent = `${lockIcon} ${network.ssid} ${authStatus} (${network.rssi}dBm)`;
            option.dataset.open = network.open;
            if (network.open) option.style.color = '#2ecc71';
            wifiForm.ssidSelect.appendChild(option);
        });
    } catch (error) {
        console.error('Erro ao buscar redes:', error);
        const option = document.createElement('option');
        option.textContent = '❌ Erro ao carregar redes';
        wifiForm.ssidSelect.appendChild(option);
    } finally {
        setLoading(false);
    }
}

async function loadWiFiSettings() {
    try {
        // Mostrar estado de carregamento
        setLoading(true);
        
        // Fazer requisição para obter as configurações
        const response = await fetch('/wifi-settings');
        
        if (!response.ok) {
            throw new Error(`Erro HTTP! status: ${response.status}`);
        }
        
        const settings = await response.json();
        console.log(settings);
        
        // Preencher o campo mDNS
        document.getElementById('dhcp-checkbox').checked = settings.dhcp || false;

        document.getElementById('mDns').value = settings.mDns || '';
        
        // Função auxiliar para dividir endereços IP
        const fillIpFields = (ipString, prefix) => {
            const parts = ipString.split('.');
            for (let i = 0; i < 4; i++) {
                const field = document.getElementById(`${prefix}${i+1}`);
                if (field) field.value = parts[i] || '0';
            }
        };

        if (!settings.dhcp) {
            staticIpForm.ipFields.forEach(field => {
                field.disabled = this.checked;
            });
        }
        
        // Preencher campos de IP
        fillIpFields(settings.ip || '192.168.1.100', 'ip');
        fillIpFields(settings.gateway || '192.168.1.1', 'gw');
        fillIpFields(settings.subnet || '255.255.255.0', 'sn');
        
        // Habilitar/desabilitar campos baseado no DHCP
        // const dhcpEnabled = settings.dhcp || false; // Assumindo que a API pode retornar isso
        // document.getElementById('dhcp-checkbox').checked = dhcpEnabled;
        
        // const ipFields = document.querySelectorAll('.ip-group input[type="number"]');
        // ipFields.forEach(field => {
        //     field.disabled = dhcpEnabled;
        // });
        
    } catch (error) {
        console.error('Erro ao carregar configurações WiFi:', error);
        showModal('Erro', 'Não foi possível carregar as configurações WiFi. Tente novamente.');
    } finally {
        setLoading(false);
    }
}

async function saveStaticIpConfig(e) {
    e.preventDefault();
    setLoading(true);

    const ssid = wifiForm.ssidSelect.value;
    const password = wifiForm.passwordInput.value;
    const dhcp = staticIpForm.dhcpCheckbox.checked;
    const mDns = staticIpForm.mDnsInput.value;
    const ip = Array.from(staticIpForm.ipFields).map(field => field.value);
    const isOpenNetwork = wifiForm.ssidSelect.options[wifiForm.ssidSelect.selectedIndex]?.dataset.open === 'true';

    // Validações iniciais
    if (!validateNetworkSelection(ssid, isOpenNetwork, password) || !validateMDns(mDns)) {
        setLoading(false);
        return;
    }

    let ipStr = "", gwStr = "", snStr = "";

    modal.saveBtn.classList.add('hidden');
    


    if (!dhcp) {
        // Validação de IP estático
        if (!isValidIP(...ip)) {
            showModal('Erro', 'Endereço IP inválido!\n- Cada octeto deve estar entre 1-254\n- Primeiro octeto não pode ser 0\n- Último octeto não pode ser 0 ou 255\n- Não pode ser 127.0.0.1');
            return;
        }
    
        // const ipStr = ip.join('.');
        ipStr = Array.from(document.querySelectorAll('.ip-group input[id^="ip"]')).map(f => f.value).join('.');
        gwStr = Array.from(document.querySelectorAll('.ip-group input[id^="gw"]')).map(f => f.value).join('.');
        snStr = Array.from(document.querySelectorAll('.ip-group input[id^="sn"]')).map(f => f.value).join('.');
        
        if (!areInSameNetwork(ipStr, gwStr, snStr)) {
            showModal('Erro', 'IP e Gateway não estão na mesma rede de acordo com a máscara fornecida!');
            setLoading(false);
            return;
        }
    
        
    } 

    showModal('Salvando', 'Aguarde enquanto salvamos as configurações...');

    try {
        const response = await fetch('/save-wifi-settings', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({
                ssid,
                password: isOpenNetwork ? '' : password,
                dhcp,
                mDns,
                ip: ipStr,
                gateway: gwStr,
                subnet: snStr
            })
        });

        if (!response.ok) throw new Error('Erro no servidor');
        
        const data = await response.json();
        showModal('Sucesso', data.message || 'Configuração salva com sucesso!');
    } catch (error) {
        showModal('Erro', 'Falha ao salvar: ' + error.message);
    } finally {
        setLoading(false);
    }
}

// =============================================
// 4. CONFIGURAÇÃO DE EVENT LISTENERS
// =============================================
// Menu responsivo
document.querySelector('.menu-toggle').addEventListener('click', () => {
    document.querySelector('nav ul').classList.toggle('active');
});

// Controle do formulário WiFi
wifiForm.ssidSelect.addEventListener('change', function() {
    const selectedOption = this.options[this.selectedIndex];
    const hasValue = this.value !== '';
    const isOpen = selectedOption?.dataset.open === 'true';

    // Esconde tudo se não tiver valor selecionado
    if (!hasValue) {
        wifiForm.passwordField.style.display = 'none';
        return;
    }

    wifiForm.passwordField.style.display = isOpen ? 'none' : 'block';
});

// Configuração de IP estático
staticIpForm.dhcpCheckbox.addEventListener('change', function() {
    staticIpForm.ipFields.forEach(field => {
        field.disabled = this.checked;
    });
});

staticIpForm.mDnsInput.addEventListener('input', function() {
    this.value = this.value.replace(/[^a-zA-Z0-9]/g, '');
});

staticIpForm.mDnsInput.addEventListener('keypress', function(e) {
    if (!/[a-zA-Z0-9]/.test(e.key)) e.preventDefault();
});

staticIpForm.ipFields.forEach((input, index) => {
    input.addEventListener('change', function() {
        if (this.value > 255) this.value = 255;
        if (this.value < 0) this.value = 0;
    });

    input.addEventListener('keydown', function(e) {
        if (['.', 'Enter', ',', 'Period'].includes(e.key)) {
            e.preventDefault();
            const nextIndex = index + 1;
            if (nextIndex < staticIpForm.ipFields.length) {
                staticIpForm.ipFields[nextIndex].focus();
                staticIpForm.ipFields[nextIndex].value = '';
            }
        }
    });
});

// Eventos principais
modal.closeBtn?.addEventListener('click', closeModal);
wifiForm.refreshBtn?.addEventListener('click', scanWifi);
staticIpForm.saveConfigBtn?.addEventListener('click', saveStaticIpConfig);
staticIpForm.saveConfigBtn?.addEventListener('click', function() {
    isSaveButtonClicked = true;
    // O setLoading(true) será chamado pela função que trata o submit
});

// Inicialização
window.addEventListener('DOMContentLoaded', () => {
    loadWiFiSettings();
    scanWifi();

});