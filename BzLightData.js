/** Affiche les détails d'un module depuis la base de données BzLightData.js */
function showModuleDetails(moduleId) {
    // 1. On récupère le numéro de série affiché
    const serial = document.getElementById('bzResSerie').innerText;
    
    // 2. On demande à ton fichier BzLightData.js les infos (Assure-toi que la fonction getRobotSpecs existe dedans)
    if (typeof bzFleetData !== 'undefined') {
        const specs = bzFleetData.getRobotSpecs(serial);
        const moduleInfo = specs[moduleId];

        if (moduleInfo) {
            const content = `
                <div class="text-left space-y-4">
                    <div class="flex items-center gap-4 border-b border-white/10 pb-4">
                        <img src="${moduleInfo.img}" class="w-16 h-16 rounded-lg border border-white/20 object-cover">
                        <div>
                            <h4 class="text-lg font-black text-white">${moduleInfo.name}</h4>
                            <span class="text-[0.6rem] font-bold uppercase text-cyan-400">Composant Système</span>
                        </div>
                    </div>
                    <div class="grid grid-cols-2 gap-4 pt-2">
                        <div>
                            <span class="block text-[0.55rem] text-gray-500 uppercase font-bold">État</span>
                            <span class="text-xs font-black text-green-400">${moduleInfo.status}</span>
                        </div>
                        <div>
                            <span class="block text-[0.55rem] text-gray-500 uppercase font-bold">Version</span>
                            <span class="text-xs font-black text-white">${moduleInfo.version || 'V2.0'}</span>
                        </div>
                    </div>
                </div>
            `;
            document.getElementById('infoModalText').innerHTML = content;
            document.getElementById('infoModal').classList.remove('hidden');
        }
    } else {
        openInfoModal("Erreur : Le fichier de données BzLightData.js est introuvable.");
    }
}

// SCRIPT ESPION POUR COORDONNÉES (À supprimer après usage)
(function() {
    const img = document.getElementById('robot-image');
    if (img) {
        img.addEventListener('click', function(e) {
            const rect = this.getBoundingClientRect();
            const x = ((e.clientX - rect.left) / rect.width) * 100;
            const y = ((e.clientY - rect.top) / rect.height) * 100;
            alert(`Top: ${y.toFixed(2)}% | Left: ${x.toFixed(2)}%`);
        });
    }
})();
