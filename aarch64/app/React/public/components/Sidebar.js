import React from 'react';
import styled from 'styled-components';
import { Menu } from 'semantic-ui-react';
import { Link } from 'react-router-dom'



const StyledSideNav = styled.div`   
    /* position: fixed;     /* Fixed Sidebar (stay in place on scroll and position relative to viewport) */
    /* height: 100%;
    /* width: 250px;     /* Set the width of the sidebar */
    /* z-index: 1;      /* Stay on top of everything */
    /* top: 54px;      /* Stay at the top */
    /* background-color: #e6e6e6; /* Axis grey */
    /* overflow-x: hidden;     /* Disable horizontal scroll */
    /* padding-top: 10px;
    /* padding-left: 10px; */
    
    .menu {
        position: fixed;
        top: 50px;
        height: 100%;
        background-color: #e6e6e6;
        border: 0px;
    }
`;

class SidebarPage extends React.Component {
    constructor(props) {
        super(props);
        this.state = {
            activeItem: 'About',
        }
    }

    // ** EDIT ** itens lists
    render() {
        const { activeItem } = this.state

        return(
          
            <StyledSideNav>
            <Menu vertical>
              
            <Menu.Item
              name='home'
              onClick={this.handleItemClick}
              as={Link}
              to="/"
            />
            <Menu.Item
              name='Hardware'
              onClick={this.handleItemClick}
              as={Link}
              to="/About"
            />
            <Menu.Item
              name='Status'
              onClick={this.handleItemClick}
              as={Link}
              to="/NoMatch"
            />
          </Menu>
          </StyledSideNav>
         
        );
    }
}


export default SidebarPage; 



