function calculateBzLightSpecs() { 
    // 1. Récupération des champs
    const companyField = document.getElementById('robotCompany');
    const serialField = document.getElementById('robotSerial');
    
    if (!companyField || !serialField) {
        console.error("Erreur : Champs de saisie introuvables dans le HTML.");
        return;
    }

    const company = companyField.value.trim();
    const serial = serialField.value.trim().toUpperCase();
    
    if (company === "") {
        openInfoModal("Veuillez renseigner votre entreprise.");
        return;
    }

    // 2. Vérification du format BZL-XX-XXXXX
    const regex = /^BZL-(25|26)-(\d{5})$/;
    if (regex.test(serial)) {
        
        // Mise à jour du texte sur le dashboard
        const displaySerial = document.getElementById('bzResSerie');
        if (displaySerial) displaySerial.innerText = serial;

        // Navigation vers le dashboard
        nav('view-bzlight-dashboard');
        
        console.log("✅ Synchronisation réussie pour : " + serial);
    } else {
        openInfoModal("Numéro de série invalide. Format requis : BZL-25-00XXX");
    }
}