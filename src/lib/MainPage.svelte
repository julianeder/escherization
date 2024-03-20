<script lang="ts">
</script>

<main>
    <div class="header">
        <h1>Eschherization</h1>
    </div>
    <slot class="mainPage"/>
    <div class="footer">
        <p>Julian Eder - TUM</p>
    </div>
</main>

<style>
    main {
        height: 100%;
        overflow-y: auto;
    }
    .mainPage {
        position: relative;
        margin: 10px;
        width: 95vw;
        height: 95vw;
    }
    .header {
        position: fixed;
        top: 0px;
        left: 0px;
        height: 50px;
        width: 100%;
    }
    .footer {
        position: fixed;
        bottom: 0px;
        left: 0px;
        height: 50px;
        width: 100%;
    }

    h1 {
        color: #ff3e00;
        font-size: 3em;
        font-weight: 100;
    }

</style>
