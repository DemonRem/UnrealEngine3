
function mainOnLoad()
{
    document.getElementById('header').innerHTML = document.title;
    document.getElementById('main').innerHTML += '<form id="xmlForm" method="POST"><input type="hidden" name="xml"/></form>';
}

