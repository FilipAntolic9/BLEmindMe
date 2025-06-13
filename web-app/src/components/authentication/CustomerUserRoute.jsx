import { Navigate } from 'react-router-dom';
import { useAuth } from '../../contexts/AuthContext';

const TenantAdminRoute = ({ children }) => {
    const { user } = useAuth();
    return user && user.role === 'CUSTOMER_USER' ? children : <Navigate to="/home" />;
};

export default TenantAdminRoute;