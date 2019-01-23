<%@ Page Language="C#" AutoEventWireup="true" CodeBehind="index.aspx.cs" Inherits="PIBWebsite.PIBPage" %>

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<html xmlns="http://www.w3.org/1999/xhtml" >
<head runat="server">
    <title>Play in Browser</title>
	<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
	<link media="all" type="text/css" rel="stylesheet" href="css/all.css" />
	<script type="text/javascript" src="js/tabs.js"></script>
	<!--[if lt IE 8]>
		<link rel="stylesheet" type="text/css" href="css/ie.css" media="screen"/>
		<script type="text/javascript" src="js/ie-png.js"></script>
	<![endif]-->
</head>
<body class="inner">
	<div id="wrapper" class="wrapper-home">
		<div class="w1">
			<!-- accessibility -->
			<ol class="accessibility">
				<li><a href="#nav">Skip to navigation</a></li>
			</ol>
			<!-- main area -->
			<div class="w2">
				<div id="main">
					<!-- visual -->
					<div class="tab-visual">
						<!-- tabs -->
						<div class="tab-holder">
							<div id="tab1" class="tab game-tab">
								<div class="tab-content">
									<h1><a href="#"><img src="images/bg-logo01.png" alt="BULLETSTORM" width="349" height="90" /></a></h1>
									<div class="tab-descr">
										<p>Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nullam lacinia neque vel tortor aliquet ut dictum mauris consectetur. Nam dignissim, velit at gravida tempor, ligula nibh pharetra odio, vitae adipiscing tellus magna vitae leo.</p>
										<div class="tab-detalis">
											<a class="trailer" href="#"><img src="images/txt-play-trailer.png" alt="play-trailer" width="132" height="16" /></a>
											<ul>
												<li><a href="#"><img src="images/txt-480.png" alt="480" width="55" height="62" /></a></li>
												<li><a href="#"><img src="images/txt-720hd.png" alt="720HD" width="85" height="62" /></a></li>
											</ul>
										</div>
										<div class="tab-more">
											<strong><a href="#"><img src="images/txt-learn-more.png" alt="image description" width="102" height="30" /></a></strong>
											<a href="#">bulletstorm.com</a>
										</div>
									</div>
								</div>
							</div>
							<div id="tab2" class="tab game-tab">
								<div class="tab-content">
									<h2><a href="#"><img src="images/bg-logo02.png" alt="GEAR OF WAR 3" width="306" height="65" /></a></h2>
									<div class="tab-descr">
										<p>Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nullam lacinia neque vel tortor aliquet ut dictum mauris consectetur. Nam dignissim, velit at gravida tempor, ligula nibh pharetra odio, vitae adipiscing tellus magna vitae leo.</p>
										<div class="tab-detalis">
											<a class="trailer" href="#"><img src="images/txt-play-trailer.png" alt="play-trailer" width="132" height="16" /></a>
											<ul>
												<li><a href="#"><img src="images/txt-480.png" alt="480" width="55" height="62" /></a></li>
												<li><a href="#"><img src="images/txt-720hd.png" alt="720HD" width="85" height="62" /></a></li>
											</ul>
										</div>
										<div class="tab-more">
											<strong><a href="#"><img src="images/txt-learn-more.png" alt="image description" width="102" height="30" /></a></strong>
											<a href="#">gearsofwar.com</a>
										</div>
									</div>
								</div>
							</div>
							<div id="tab3" class="tab game-tab">
								<div class="tab-content">
									<h2><a href="#"><img src="images/bg-logo03.png" alt="SHADOW COMPLEX" width="348" height="167" /></a></h2>
									<div class="tab-descr">
										<p>Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nullam lacinia neque vel tortor aliquet ut dictum mauris consectetur. Nam dignissim, velit at gravida tempor, ligula nibh pharetra odio, vitae adipiscing tellus magna vitae leo.</p>
										<div class="tab-detalis">
											<a class="trailer" href="#"><img src="images/txt-play-trailer.png" alt="play-trailer" width="132" height="16" /></a>
											<ul>
												<li><a href="#"><img src="images/txt-480.png" alt="480" width="55" height="62" /></a></li>
												<li><a href="#"><img src="images/txt-720hd.png" alt="720HD" width="85" height="62" /></a></li>
											</ul>
										</div>
										<div class="tab-more">
											<strong><a href="#"><img src="images/txt-learn-more.png" alt="image description" width="102" height="30" /></a></strong>
											<a href="#">shdowcomplex.com</a>
										</div>
									</div>
								</div>
							</div>
							<div id="tab4" class="tab game-tab">
								<form id="PIBForm" runat="server">
								    <PIB:PIBPanel ID="GamePanel" runat="Server" Width="800" Height="600" BackColor="Gray" />
								</form>
							</div>
							<!-- tabset -->
							<div class="tabset">
								<ul>
									<li>
										<a href="#tab1" class="tab active"><img src="images/bg-tab01.png" alt="BULLETSTORM" width="233" height="162" /></a>
										<span><a href="#">play</a></span>
									</li>
									<li>
										<a href="#tab2" class="tab"><img src="images/bg-tab02.png" alt="GEAR OF WAR 3" width="241" height="162" /></a>
										<span><a href="#">play</a></span>
									</li>
									<li>
										<a href="#tab3" class="tab"><img src="images/bg-tab03.png" alt="SHADOW COMPLEX" width="237" height="162" /></a>
										<span><a href="#">play</a></span>
									</li>
									<li>
										<a href="#tab4" class="tab"><img src="images/bg-tab04.png" alt="UDK" width="235" height="162" /></a>
										<span><a href="#">play</a></span>
									</li>
								</ul>
							</div>
						</div>
					</div>
					<!-- featured item -->
					<div class="featured-item">
						<!-- enlist -->
						<div class="enlist">
							<div class="fi-head">
								<h2><img src="images/txt-enlist.png" alt="enlist" width="68" height="11" /></h2>
								<ul>
									<li><a href="#">See All Employment</a></li>
								</ul>
							</div>
							<ul class="el-list">
								<li>
									<div class="img-shdw">
										<a href="#"><img src="images/img05.jpg" alt="image description" width="67" height="63" /></a>
									</div>
									<div class="el-text">
										<h3><a href="#">Pixel god</a></h3>
										<p>Job title rem ipsum dolor sit amet, consectetur adipis cing.</p>
										<a class="apply" href="#"><img src="images/txt-apply.png" alt="image description" width="47" height="22" /></a>
									</div>
								</li>
								<li>
									<div class="img-shdw">
										<a href="#"><img src="images/img06.jpg" alt="image description" width="67" height="63" /></a>
									</div>
									<div class="el-text">
										<h3><a href="#">Digital Mercentary</a></h3>
										<p>Job title rem ipsum dolor sit amet, consectetur adipis cing elit m ipsum.</p>
										<a class="apply" href="#"><img src="images/txt-apply.png" alt="image description" width="47" height="22" /></a>
									</div>
								</li>
							</ul>
						</div>
						<!-- news -->
						<div class="f-news">
							<div class="fi-head">
								<a class="f-soc" href="#"><img src="images/rss.gif" alt="rss" width="34" height="31" /></a>
								<h2><img src="images/txt-news.png" alt="news from the front" width="206" height="11" /></h2>
								<ul>
									<li><a href="#">See All</a></li>
									<li><a href="#">Subscribe</a></li>
								</ul>
							</div>
							<ul class="news-list">
								<li>
									<em class="date">04.13.10</em>
									<a href="#">Uber Entertainment Licenses Unreal Engine 3</a>
								</li>
								<li>
									<em class="date">04.13.10</em>
									<a href="#">Gears of War 3 Ignites Anticipation as Biggest 	Blockbuster Game of 2011</a>
								</li>
								<li>
									<em class="date">04.13.10</em>
									<a href="#">Gallery Books to Publish Official Licensed Tie-Ins 	to Gears of War</a>
								</li>
								<li>
									<em class="date">04.13.10</em>
									<a href="#">Epic Games, Inc. Opens New Subsidiary in Japan</a>
								</li>
							</ul>
						</div>
						<!-- transponder -->
						<div class="transponder">
							<div class="fi-head">
								<a class="f-soc" href="#"><img src="images/twitter01.gif" alt="image description" width="34" height="31" /></a>
								<h2><img src="images/txt-trans.png" alt="transponder" width="133" height="11" /></h2>
								<ul>
									<li><a href="#">See Feed</a></li>
								</ul>
							</div>
							<ul class="trans-list">
								<li>
									<div class="avatar">
										<a href="#"><img src="images/img07.jpg" alt="image description" width="33" height="34" /></a>
									</div>
									<div class="trans-text">
										<blockquote>
											<div>
												<q>
													<img src="images/quot01.gif" alt="&quot;" width="11" height="9" /> 
													Lorem ipsum dolor sit amet, conse ctetur adipiscing elit. Vestibulum dui arcu, volutpat vel suscipit in. <a class="link-home" href="#">#home</a> 
													<img src="images/quot02.gif" alt="&quot;" width="10" height="9" />
												</q>
											</div>
										</blockquote>
										<div class="links-hold">
											<span><a href="#">@ Cliff Bleszinski</a></span>
											<a href="#">Follow</a>
										</div>
									</div>
								</li>
								<li>
									<div class="avatar">
										<a href="#"><img src="images/img08.jpg" alt="image description" width="33" height="34" /></a>
									</div>
									<div class="trans-text">
										<blockquote>
											<div>
												<q>
													<img src="images/quot01.gif" alt="&quot;" width="11" height="9" />
													Epic Games, Inc. has released the April 2010 Unreal Development Kit (UDK) Beta! <a href="#">http://tinyurl.com/3xa7k</a> <a class="link-home" href="#">#home</a>
													<img src="images/quot02.gif" alt="&quot;" width="10" height="9" />
												</q>
											</div>
										</blockquote>
										<div class="links-hold">
											<span><a href="#">@ Epic Games</a></span>
											<a href="#">Follow</a>
										</div>
									</div>
								</li>
							</ul>
						</div>
					</div>
				</div>
				<!-- header -->
				<div id="header">
					<!-- logo -->
					<strong class="logo"><a href="#" accesskey="1">Epic Games</a></strong>
					<!-- main navigation -->
					<ul id="nav">
						<li class="active"><a href="#">HOME</a></li>
						<li><a href="#">ABOUT</a></li>
						<li><a href="#">GAMES</a></li>
						<li><a href="#">TECHNOLOGY</a></li>
						<li><a href="#">CAREERS</a></li>
						<li><a href="#">NEWS &amp; EVENTS</a></li>
						<li><a href="#">CONTACT</a></li>
					</ul>
					<!-- search form -->
					<form class="search-form" action="#">
						<fieldset>
							<label for="search-text" title="search"><span class="access">search</span><a href="#"><img src="images/txt-search.png" alt="image description" width="46" height="30" /></a></label>
							<span class="search-text"><input id="search-text" type="text" value="" /></span>
							<input type="submit" title="submit" class="access" />
						</fieldset>
					</form>
				</div>
			</div>
		</div>
		<!-- footer -->
		<div id="footer-inner">
			<div class="menu-bar">
				<div class="holder">
					<a href="#" class="epic-games"><img src="images/txt-epic.gif" alt="image description" width="79" height="32" /></a>
					<!-- social -->
					<ul class="social">
						<li><a href="#"><img src="images/facebook.gif" alt="facebook" width="21" height="42" /></a></li>
						<li><a href="#"><img src="images/twitter.gif" alt="twitter" width="21" height="42" /></a></li>
						<li><a href="#"><img src="images/you-tube.gif" alt="youtube" width="21" height="42" /></a></li>
					</ul>
					<!-- top navigation -->
					<ul class="navigation">
						<li>
							<div class="drpdwn">
								<ul>
									<li><a href="#">Subnav Item</a></li>
									<li><a href="#">Subnav Item</a></li>
									<li><a href="#">Subnav Item</a></li>
								</ul>
								<div class="dd-b">&nbsp;</div>
							</div>
							<a href="#"><img src="images/txt-games.gif" alt="games" width="101" height="42" /></a>
						</li>
						<li><a href="#"><img src="images/txt-technology.gif" alt="technology" width="101" height="42" /></a></li>
						<li><a href="#"><img src="images/txt-community.gif" alt="community" width="101" height="42" /></a></li>
					</ul>
				</div>
			</div>
		</div>
	</div>
	<div class="bg-page p-inner"><div>&nbsp;</div></div>
</body>
</html>
