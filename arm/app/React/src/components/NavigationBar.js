import React from 'react';
import { Nav, Navbar, Form, FormControl } from 'react-bootstrap';
import styled from 'styled-components';
import {acapName} from '../App';

const Styles = styled.div`
  .navbar { 
    background-color: white; 
    min-width:100px
  }
  a, .navbar-light {
    height: 34px;
    width: 100%; 
    color: black;
    &:hover { color: white; }
  }
  .nav-link {
    color: black;
    &:hover { color: black; }
  }
  .navbar-brand {
    padding-top: 1rem;
    font-size: 1.4em;
    color: black;
    margin-left: 100px;
    &:hover { color: black; }
  }
  .navbar-collapse {
    position: fixed;     /* Fixed Sidebar (stay in place on scroll and position relative to viewport) */
    top:0px; 
    left: 85%
    width:100%; 
    height:47px;
    background-color: #FFCC33; /* Axis yeallow */
    font-size: 1.4em;
    color: black;
    &:hover { color: white; }
  }
  .form-center {
    position: absolute !important;
    left: 25%;
    right: 25%;
  }
  #conertext {
    padding: 25px;
    color:grey;
    padding-top: 27px;
    font-size: 10px;
  }
  #line {
    top:0px;
    height: 6px;
    background-color: #e6e6e6; 
  }
  #cornerimage {
    top: 0px;
    height: 62px;
  }
  #cornerinfo{
    position: fixed;
    height:62px;
    width:100%;
    background-color: #FFCC33; /* Axis yeallow */
    left: 85%;
    font-size: 1.4em;
  }
  #logoimage{
    position: fixed;
    top:5px;
    font-size: 1.4em;
  }
`;


export const NavigationBar = () => (
  <Styles>
    <Navbar >
      <img id="logoimage" src="./img/logo.svg" ></img>
      <Navbar.Brand>{acapName}</Navbar.Brand>
      <div id="cornerinfo">
        <img id="cornerimage" src="./img/topcorner62.png" alt="Axis" ></img>
        {/*<span id="conertext">Professional Services US</span>*/}
      </div>
    </Navbar>
      <hr id="line"></hr>
  </Styles>
)

/*
      <Form className="form-center">
        <FormControl type="text" placeholder="Search" className="" />
      </Form>
*/